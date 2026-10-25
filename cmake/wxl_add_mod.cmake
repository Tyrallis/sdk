# SPDX-License-Identifier: MIT
# Copyright (c) 2026 WarcraftXL
#
# wxl_add_mod(): one source, one command, two binaries.
#
#   wxl_add_mod(<name>
#       SOURCES <file>...
#       [MANIFEST <mod.json>]            # default: mod.json beside the calling CMakeLists.txt
#       [TARGETS <win-x64|win-x86>...]   # default: both
#       [OUTPUT_NAME <base>]             # default: <name>; the DLL is <base>.dll
#       [OUTPUT_DIR <dir>]               # default: ${CMAKE_BINARY_DIR}/dist
#       [CXX_STANDARD <17|20|23>])       # default: 20
#
# Produces, from `cmake -S . -B build && cmake --build build`:
#
#   <OUTPUT_DIR>/<manifest id>/
#     mod.json
#     bin/x64/<base>.dll        win-x64
#     bin/x86/<base>.dll        win-x86
#
# One CMake build tree is one architecture, so the call plays three roles:
#
#   - A Visual Studio generator, WXL_MOD_ARCH unset (the mod author's `cmake -S . -B build`): a
#     superbuild. For every requested architecture it configures and builds the same source
#     directory as a child project (ExternalProject) with WXL_MOD_ARCH set, selecting the
#     architecture with the generator platform. Each child installs its DLL into the mod folder;
#     the manifest is copied next to them.
#   - Any other generator (Ninja, Makefiles), WXL_MOD_ARCH unset: the one architecture the
#     compiler targets, built in this tree into the same mod folder. Run it once per architecture
#     with the same OUTPUT_DIR, or install each tree into one folder, to fill the folder.
#   - WXL_MOD_ARCH set (a child, or a tree a runtime's own build drives): the real library target
#     <name>, linking wxl::sdk-mod, with an install rule into bin/<arch>/.
#
# A mod is C++20 unless it says otherwise: wxl::Log formats with std::format, which C++20 brings.
# The SDK headers take C++17, for a mod whose code is written to it. Anything else the mod wants
# on its target goes inside `if(TARGET <name>)`: at the top level of the superbuild there is no
# library target, only the per-architecture drivers.
#
# Cache variables:
#   WXL_MOD_TARGETS  overrides TARGETS from the command line, e.g. -DWXL_MOD_TARGETS=win-x64.
#   WXL_MOD_ARCH     the architecture of this tree; set by the superbuild, or by hand.
#
# Works from a checkout (<sdk>/cmake/wxl_add_mod.cmake) and from an installed SDK, where this file
# sits next to wxl-sdk-targets.cmake and is included by wxl-sdk-config.cmake.

include_guard(GLOBAL)

set(_WXL_MOD_MODULE_DIR "${CMAKE_CURRENT_LIST_DIR}")
set(_WXL_MOD_KNOWN_ARCHS win-x64 win-x86)

set(WXL_MOD_TARGETS "" CACHE STRING
    "Architectures to build (win-x64;win-x86). Empty takes the TARGETS of wxl_add_mod.")
set(WXL_MOD_ARCH "" CACHE STRING
    "Architecture of this build tree. Set by the superbuild; leave empty to build every target.")

# A child tree compiles, and so does a tree of a generator that builds one architecture; the
# Visual Studio superbuild above its children never needs a compiler. The language is enabled
# here, at file scope, so the mod's project() may say LANGUAGES NONE.
if(WXL_MOD_ARCH)
    if(NOT WXL_MOD_ARCH IN_LIST _WXL_MOD_KNOWN_ARCHS)
        message(FATAL_ERROR "WXL_MOD_ARCH is '${WXL_MOD_ARCH}'; it must be one of: ${_WXL_MOD_KNOWN_ARCHS}")
    endif()
    enable_language(CXX)
elseif(NOT CMAKE_GENERATOR MATCHES "^Visual Studio")
    enable_language(CXX)
endif()

# --- Per-architecture facts -------------------------------------------------------------------

# win-x64 -> x64, win-x86 -> x86: the folder under bin/.
function(_wxl_mod_arch_dir arch out)
    string(REGEX REPLACE "^win-" "" dir "${arch}")
    set(${out} "${dir}" PARENT_SCOPE)
endfunction()

# win-x64 -> x64, win-x86 -> Win32: the Visual Studio generator platform.
function(_wxl_mod_vs_platform arch out)
    if(arch STREQUAL "win-x64")
        set(${out} "x64" PARENT_SCOPE)
    else()
        set(${out} "Win32" PARENT_SCOPE)
    endif()
endfunction()

# win-x64 -> 8, win-x86 -> 4: what CMAKE_SIZEOF_VOID_P must report in the child.
function(_wxl_mod_pointer_size arch out)
    if(arch STREQUAL "win-x64")
        set(${out} 8 PARENT_SCOPE)
    else()
        set(${out} 4 PARENT_SCOPE)
    endif()
endfunction()

# --- The SDK targets (child role) -------------------------------------------------------------

# Makes wxl::sdk-mod available: from the checkout this file lives in, or from the installed
# export next to it.
function(_wxl_mod_provide_sdk)
    if(TARGET wxl::sdk-mod)
        return()
    endif()
    if(EXISTS "${_WXL_MOD_MODULE_DIR}/../include/wxl/abi.h")
        set(WXL_SDK_TESTS OFF CACHE BOOL "" FORCE)
        add_subdirectory("${_WXL_MOD_MODULE_DIR}/.." "${CMAKE_BINARY_DIR}/wxl-sdk" EXCLUDE_FROM_ALL)
    elseif(EXISTS "${_WXL_MOD_MODULE_DIR}/wxl-sdk-targets.cmake")
        include("${_WXL_MOD_MODULE_DIR}/wxl-sdk-targets.cmake")
    else()
        message(FATAL_ERROR "wxl_add_mod: cannot find the SDK next to ${_WXL_MOD_MODULE_DIR}: "
            "neither ../include/wxl/abi.h (a checkout) nor wxl-sdk-targets.cmake (an install).")
    endif()
endfunction()

# --- The manifest (superbuild role) -----------------------------------------------------------

# Reads one string property out of a piece of the manifest with a plain regex. Strict on
# purpose: the manifest is small and a JSON parser is a dependency the SDK does not take.
function(_wxl_mod_manifest_string json key out)
    if(json MATCHES "\"${key}\"[ \t\r\n]*:[ \t\r\n]*\"([^\"]*)\"")
        set(${out} "${CMAKE_MATCH_1}" PARENT_SCOPE)
    else()
        set(${out} "" PARENT_SCOPE)
    endif()
endfunction()

# Checks the manifest against what this build produces and returns its id. Every architecture
# built here (`archs`) must be promised at the path it lands on, and every Windows architecture
# promised must be one the mod is built for (`declared`, the TARGETS of wxl_add_mod): a runtime
# takes the manifest at its word. One that is declared but not built by this tree -- another
# tree's, or one left out with WXL_MOD_TARGETS -- is only noted: the folder lacks it until then.
function(_wxl_mod_check_manifest manifest archs declared output_name out_id)
    if(NOT EXISTS "${manifest}")
        message(FATAL_ERROR "wxl_add_mod: manifest not found: ${manifest}")
    endif()
    file(READ "${manifest}" json)

    _wxl_mod_manifest_string("${json}" id id)
    if(NOT id MATCHES "^[a-z0-9]+([.-][a-z0-9]+)*$")
        message(FATAL_ERROR "wxl_add_mod: ${manifest}: \"id\" is missing or malformed "
            "(got '${id}'; the schema wants [a-z0-9.-], e.g. \"wxl.example\").")
    endif()

    # The contract version the headers carry has to be the one the manifest claims.
    _wxl_mod_manifest_string("${json}" abi abi)
    if(EXISTS "${_WXL_MOD_MODULE_DIR}/../include/wxl/abi.h")
        set(abi_h "${_WXL_MOD_MODULE_DIR}/../include/wxl/abi.h")
    else()
        set(abi_h "${_WXL_MOD_MODULE_DIR}/../../../include/wxl/abi.h")
    endif()
    if(EXISTS "${abi_h}")
        file(READ "${abi_h}" abi_src)
        string(REGEX MATCH "WXL_ABI_VERSION_MAJOR[ \t]+([0-9]+)" _ "${abi_src}")
        set(abi_major "${CMAKE_MATCH_1}")
        string(REGEX MATCH "WXL_ABI_VERSION_MINOR[ \t]+([0-9]+)" _ "${abi_src}")
        set(abi_minor "${CMAKE_MATCH_1}")
        string(REGEX MATCH "WXL_ABI_VERSION_FEATURE[ \t]+([0-9]+)" _ "${abi_src}")
        set(abi_feature "${CMAKE_MATCH_1}")
        set(abi_headers "${abi_major}.${abi_minor}.${abi_feature}")
        # The manifest names the lowest version the mod needs: the headers' major.minor, and a
        # feature no higher than theirs.
        if(abi MATCHES "^([0-9]+)\\.([0-9]+)$")
            message(FATAL_ERROR "wxl_add_mod: ${manifest}: \"abi\" is \"${abi}\", the two-part "
                "numbering the SDK used before 0.1.5. Set \"abi\": \"${abi_headers}\".")
        endif()
        set(abi_ok FALSE)
        if(abi MATCHES "^([0-9]+)\\.([0-9]+)\\.([0-9]+)$")
            if(CMAKE_MATCH_1 EQUAL abi_major AND CMAKE_MATCH_2 EQUAL abi_minor
               AND NOT CMAKE_MATCH_3 GREATER abi_feature)
                set(abi_ok TRUE)
            endif()
        endif()
        if(NOT abi_ok)
            message(FATAL_ERROR "wxl_add_mod: ${manifest}: \"abi\" is \"${abi}\" but the SDK headers "
                "are ${abi_headers}: the manifest can ask for ${abi_major}.${abi_minor}.0 to "
                "${abi_headers}. Set \"abi\": \"${abi_headers}\".")
        endif()
    endif()

    if(NOT json MATCHES "\"binaries\"[ \t\r\n]*:[ \t\r\n]*{([^}]*)}")
        message(FATAL_ERROR "wxl_add_mod: ${manifest}: no \"binaries\" object.")
    endif()
    set(binaries "${CMAKE_MATCH_1}")

    foreach(arch IN LISTS _WXL_MOD_KNOWN_ARCHS)
        _wxl_mod_arch_dir("${arch}" dir)
        set(expected "bin/${dir}/${output_name}.dll")
        _wxl_mod_manifest_string("${binaries}" "${arch}" promised)

        if(arch IN_LIST archs)
            if(NOT promised)
                message(FATAL_ERROR "wxl_add_mod: ${manifest} promises no binary for ${arch}, "
                    "which this build produces.\n"
                    "  Add to \"binaries\":   \"${arch}\": \"${expected}\"\n"
                    "  or leave it out:       -DWXL_MOD_TARGETS=<the others>")
            elseif(NOT promised STREQUAL expected)
                message(FATAL_ERROR "wxl_add_mod: ${manifest} promises ${arch} at \"${promised}\" "
                    "but the build writes it to \"${expected}\". A runtime would look in the "
                    "wrong place: make the two agree.")
            endif()
        elseif(promised AND arch IN_LIST declared)
            message(STATUS "wxl_add_mod: ${arch} is not built by this tree: the mod folder lacks "
                "${promised} until a build for ${arch} writes it there")
        elseif(promised)
            message(FATAL_ERROR "wxl_add_mod: ${manifest} promises \"${arch}\": \"${promised}\" "
                "but the mod is not built for ${arch} (TARGETS: ${declared}). A runtime on that "
                "architecture would load a file that is not there.\n"
                "  Build it:           add ${arch} to the TARGETS of wxl_add_mod\n"
                "  or stop promising:  remove \"${arch}\" from \"binaries\"")
        endif()
    endforeach()

    set(${out_id} "${id}" PARENT_SCOPE)
endfunction()

# --- The toolchain (superbuild role) ----------------------------------------------------------

# Fails before the child does, with the one message that matters, when the Visual Studio in use
# has no compiler for the architecture.
function(_wxl_mod_check_toolchain arch)
    if(NOT CMAKE_GENERATOR MATCHES "^Visual Studio" OR NOT CMAKE_GENERATOR_INSTANCE)
        return()
    endif()
    string(REGEX REPLACE ",.*$" "" vs_root "${CMAKE_GENERATOR_INSTANCE}")
    _wxl_mod_arch_dir("${arch}" dir)
    file(GLOB found "${vs_root}/VC/Tools/MSVC/*/bin/Host*/${dir}/cl.exe")
    if(found)
        return()
    endif()
    if(arch STREQUAL "win-x86")
        set(bits "32-bit (x86)")
    else()
        set(bits "64-bit (x64)")
    endif()
    message(FATAL_ERROR "wxl_add_mod: the ${bits} MSVC toolchain is not installed in\n"
        "  ${vs_root}\n"
        "so ${arch} cannot be built. Install it: Visual Studio Installer > Modify > Individual "
        "components > \"MSVC v14x - VS 20xx C++ x64/x86 build tools (Latest)\" (one component "
        "carries both targets), then configure again.\n"
        "To build the others without it: -DWXL_MOD_TARGETS=<the others>; the mod folder then "
        "lacks ${arch} until a build that has that toolchain writes it.")
endfunction()

# --- wxl_add_mod ------------------------------------------------------------------------------

function(wxl_add_mod name)
    cmake_parse_arguments(PARSE_ARGV 1 arg "" "MANIFEST;OUTPUT_NAME;OUTPUT_DIR;CXX_STANDARD"
        "SOURCES;TARGETS")

    if(arg_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "wxl_add_mod(${name}): unknown arguments: ${arg_UNPARSED_ARGUMENTS}")
    endif()
    if(NOT arg_SOURCES)
        message(FATAL_ERROR "wxl_add_mod(${name}): SOURCES is required")
    endif()
    if(NOT arg_MANIFEST)
        set(arg_MANIFEST mod.json)
    endif()
    if(NOT arg_OUTPUT_NAME)
        set(arg_OUTPUT_NAME "${name}")
    endif()
    if(NOT arg_OUTPUT_DIR)
        set(arg_OUTPUT_DIR "${CMAKE_BINARY_DIR}/dist")
    endif()
    if(NOT arg_TARGETS)
        set(arg_TARGETS ${_WXL_MOD_KNOWN_ARCHS})
    endif()
    if(NOT arg_CXX_STANDARD)
        set(arg_CXX_STANDARD 20)
    elseif(NOT arg_CXX_STANDARD MATCHES "^(17|20|23|26)$")
        message(FATAL_ERROR "wxl_add_mod(${name}): CXX_STANDARD is '${arg_CXX_STANDARD}'; the SDK "
            "headers take 17 or later (17, 20, 23)")
    endif()
    set(_WXL_MOD_CXX_STANDARD "${arg_CXX_STANDARD}")
    get_filename_component(manifest "${arg_MANIFEST}" ABSOLUTE BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")

    if(WXL_MOD_ARCH)
        _wxl_mod_add_library("${name}" "${arg_OUTPUT_NAME}" "${arg_SOURCES}")
    elseif(CMAKE_GENERATOR MATCHES "^Visual Studio")
        _wxl_mod_add_superbuild("${name}" "${arg_OUTPUT_NAME}" "${manifest}" "${arg_TARGETS}"
            "${arg_OUTPUT_DIR}")
    else()
        _wxl_mod_add_single("${name}" "${arg_OUTPUT_NAME}" "${manifest}" "${arg_TARGETS}"
            "${arg_OUTPUT_DIR}" "${arg_SOURCES}")
    endif()
endfunction()

# The architecture the compiler of this tree targets: win-x64 or win-x86.
function(_wxl_mod_compiler_arch out)
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(${out} win-x64 PARENT_SCOPE)
    else()
        set(${out} win-x86 PARENT_SCOPE)
    endif()
endfunction()

# Single-tree role: a generator that cannot switch architecture builds the one its compiler
# targets, as a library target of this tree, straight into the mod folder.
function(_wxl_mod_add_single name output_name manifest default_targets output_dir sources)
    _wxl_mod_compiler_arch(arch)
    if(WXL_MOD_TARGETS)
        set(wanted ${WXL_MOD_TARGETS})
    else()
        set(wanted ${default_targets})
    endif()
    foreach(one IN LISTS wanted)
        if(NOT one IN_LIST _WXL_MOD_KNOWN_ARCHS)
            message(FATAL_ERROR "wxl_add_mod(${name}): unknown target '${one}'; "
                "the choices are: ${_WXL_MOD_KNOWN_ARCHS}")
        endif()
    endforeach()

    if(NOT arch IN_LIST wanted)
        if(WXL_MOD_TARGETS)
            message(FATAL_ERROR "wxl_add_mod(${name}): this tree's compiler targets ${arch}, and "
                "'${CMAKE_GENERATOR}' builds that one architecture only, but WXL_MOD_TARGETS asks "
                "for ${wanted}. Configure this tree with a compiler for ${wanted}: on Windows from "
                "the matching \"Native Tools Command Prompt\", from Linux with that architecture's "
                "toolchain file. Or use a Visual Studio generator, which builds both from one tree.")
        endif()
        message(STATUS "${name}: built for ${wanted} only, and this tree's compiler targets "
            "${arch}: nothing to build here")
        return()
    endif()

    _wxl_mod_check_manifest("${manifest}" "${arch}" "${default_targets}" "${output_name}" id)
    set(mod_dir "${output_dir}/${id}")
    _wxl_mod_arch_dir("${arch}" dir)

    set(WXL_MOD_ARCH "${arch}")
    _wxl_mod_add_library("${name}" "${output_name}" "${sources}")
    # The DLL lands in the mod folder as it is built, the manifest beside it, as the superbuild
    # lays them out. `cmake --install` still writes bin/<arch>/ under the prefix it is given.
    set_target_properties(${name} PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY "${mod_dir}/bin/${dir}"
        LIBRARY_OUTPUT_DIRECTORY_DEBUG "${mod_dir}/bin/${dir}"
        LIBRARY_OUTPUT_DIRECTORY_RELEASE "${mod_dir}/bin/${dir}"
        LIBRARY_OUTPUT_DIRECTORY_RELWITHDEBINFO "${mod_dir}/bin/${dir}"
        LIBRARY_OUTPUT_DIRECTORY_MINSIZEREL "${mod_dir}/bin/${dir}")
    add_custom_command(TARGET ${name} POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${manifest}" "${mod_dir}/mod.json"
        COMMENT "${name}: mod.json -> ${mod_dir}")
    message(STATUS "${name}: ${arch}  -> ${mod_dir}/bin/${dir}/${output_name}.dll "
        "(one architecture per tree with '${CMAKE_GENERATOR}')")
endfunction()

# Child role: the library itself.
function(_wxl_mod_add_library name output_name sources)
    _wxl_mod_pointer_size("${WXL_MOD_ARCH}" want)
    if(NOT CMAKE_SIZEOF_VOID_P EQUAL want)
        math(EXPR have "${CMAKE_SIZEOF_VOID_P} * 8")
        math(EXPR want_bits "${want} * 8")
        message(FATAL_ERROR "wxl_add_mod(${name}): this tree is WXL_MOD_ARCH=${WXL_MOD_ARCH} "
            "(${want_bits}-bit) but the compiler targets ${have}-bit. With a Visual Studio "
            "generator pass the matching platform (-A x64 or -A Win32); with another generator, "
            "configure from a developer prompt of the right architecture.")
    endif()

    _wxl_mod_provide_sdk()
    _wxl_mod_arch_dir("${WXL_MOD_ARCH}" dir)

    add_library(${name} MODULE ${sources})
    target_link_libraries(${name} PRIVATE wxl::sdk-mod)
    set_target_properties(${name} PROPERTIES
        OUTPUT_NAME "${output_name}"
        PREFIX ""
        CXX_STANDARD ${_WXL_MOD_CXX_STANDARD}
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF)
    if(MSVC)
        target_compile_options(${name} PRIVATE /W4 /permissive-)
    else()
        target_compile_options(${name} PRIVATE -Wall -Wextra)
    endif()

    # `cmake --install` (and the superbuild's install step) puts the DLL where the manifest says.
    install(TARGETS ${name} LIBRARY DESTINATION "bin/${dir}")
endfunction()

# Superbuild role: one child per architecture, then the manifest next to them.
function(_wxl_mod_add_superbuild name output_name manifest default_targets output_dir)
    if(WXL_MOD_TARGETS)
        set(archs ${WXL_MOD_TARGETS})
    else()
        set(archs ${default_targets})
    endif()
    foreach(arch IN LISTS archs)
        if(NOT arch IN_LIST _WXL_MOD_KNOWN_ARCHS)
            message(FATAL_ERROR "wxl_add_mod(${name}): unknown target '${arch}'; "
                "the choices are: ${_WXL_MOD_KNOWN_ARCHS}")
        endif()
    endforeach()

    _wxl_mod_check_manifest("${manifest}" "${archs}" "${default_targets}" "${output_name}" id)
    set(mod_dir "${output_dir}/${id}")

    # Only a Visual Studio generator gets here: it takes the platform as a parameter, per child.

    # Settings the children must see as this tree saw them.
    set(forward)
    foreach(var WXL_SDK_DIR wxl-sdk_DIR CMAKE_PREFIX_PATH CMAKE_BUILD_TYPE)
        if(DEFINED ${var})
            list(APPEND forward "-D${var}=${${var}}")
        endif()
    endforeach()
    set(generator_args CMAKE_GENERATOR "${CMAKE_GENERATOR}")
    if(CMAKE_GENERATOR_TOOLSET)
        list(APPEND generator_args CMAKE_GENERATOR_TOOLSET "${CMAKE_GENERATOR_TOOLSET}")
    endif()
    if(CMAKE_GENERATOR_INSTANCE)
        list(APPEND generator_args CMAKE_GENERATOR_INSTANCE "${CMAKE_GENERATOR_INSTANCE}")
    endif()

    include(ExternalProject)
    set(children)

    foreach(arch IN LISTS _WXL_MOD_KNOWN_ARCHS)
        _wxl_mod_arch_dir("${arch}" dir)
        if(NOT arch IN_LIST archs)
            message(STATUS "${name}: ${arch}  skipped (not in the targets: ${archs})")
            continue()
        endif()
        _wxl_mod_check_toolchain("${arch}")

        _wxl_mod_vs_platform("${arch}" platform)
        set(platform_args CMAKE_GENERATOR_PLATFORM "${platform}")

        # BUILD_ALWAYS hands every build to the child, whose own dependency tracking decides
        # whether anything is recompiled; the configure step runs once and the child
        # reconfigures itself when its CMakeLists.txt changes.
        ExternalProject_Add(${name}-${arch}
            SOURCE_DIR  "${CMAKE_CURRENT_SOURCE_DIR}"
            BINARY_DIR  "${CMAKE_CURRENT_BINARY_DIR}/${arch}"
            PREFIX      "${CMAKE_CURRENT_BINARY_DIR}/${arch}-steps"
            INSTALL_DIR "${mod_dir}"
            ${generator_args}
            ${platform_args}
            CMAKE_ARGS
                "-DWXL_MOD_ARCH:STRING=${arch}"
                "-DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>"
                ${forward}
            BUILD_ALWAYS ON
            BUILD_BYPRODUCTS "${mod_dir}/bin/${dir}/${output_name}.dll")
        list(APPEND children ${name}-${arch})
        message(STATUS "${name}: ${arch}  -> ${mod_dir}/bin/${dir}/${output_name}.dll")
    endforeach()

    add_custom_command(
        OUTPUT  "${mod_dir}/mod.json"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${manifest}" "${mod_dir}/mod.json"
        DEPENDS "${manifest}"
        COMMENT "${name}: mod.json -> ${mod_dir}")

    # The mod folder: every child plus the manifest.
    add_custom_target(${name}-dist ALL
        DEPENDS ${children} "${mod_dir}/mod.json")
    message(STATUS "${name}: mod folder ${mod_dir}")
endfunction()

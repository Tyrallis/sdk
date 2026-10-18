cmake_minimum_required(VERSION 3.21)

# wxl.lua: the Lua mod runtime, itself a mod built with wxl_add_mod. From this directory it is
# a superbuild producing dist/wxl.lua/ for both architectures; from the SDK's own tree, which
# sets WXL_MOD_ARCH, it is the library target of that tree's architecture.
project(wxl-lua VERSION 0.1.0 LANGUAGES NONE)

set(WXL_SDK_DIR "${CMAKE_CURRENT_SOURCE_DIR}/.." CACHE PATH "Path to the WarcraftXL SDK")
include("${WXL_SDK_DIR}/cmake/wxl_add_mod.cmake")

wxl_add_mod(wxl-lua
    SOURCES
        src/Args.cpp
        src/LuaMod.cpp
        src/Objects.cpp
        src/Runtime.cpp
        src/Sandbox.cpp
        src/ScriptMod.cpp
    MANIFEST mod.json
    OUTPUT_NAME wxl-lua
    TARGETS win-x64 win-x86)

if(TARGET wxl-lua)
    include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/luajit.cmake")

    # The host library for the manifest parser and the load order; the runtime headers for
    # object_reads.def, which the object methods are generated from.
    target_link_libraries(wxl-lua PRIVATE wxl::host luajit::lib)
    target_include_directories(wxl-lua PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
        "${WXL_SDK_DIR}/runtime/include")

    # /EHs: a Lua error unwinds through C++ frames by way of C functions, and their destructors
    # must run. The dynamic CRT in every configuration, to match the LuaJIT objects.
    if(MSVC)
        target_compile_options(wxl-lua PRIVATE /EHs)
        set_target_properties(wxl-lua PROPERTIES MSVC_RUNTIME_LIBRARY "MultiThreadedDLL")
    endif()

    # The editor definitions, generated from the same tables and kept in types/ for an editor
    # to point at. The build rewrites the file when the tables moved; tests/ checks it matches.
    add_executable(wxl-lua-types tools/gen_types.cpp)
    target_link_libraries(wxl-lua-types PRIVATE wxl::sdk)
    target_include_directories(wxl-lua-types PRIVATE "${WXL_SDK_DIR}/runtime/include")
    if(MSVC)
        target_compile_options(wxl-lua-types PRIVATE /W4 /permissive-)
    else()
        target_compile_options(wxl-lua-types PRIVATE -Wall -Wextra)
    endif()
    add_custom_target(wxl-lua-types-gen ALL
        COMMAND wxl-lua-types "${CMAKE_CURRENT_SOURCE_DIR}/types/wxl.d.lua"
        COMMENT "wxl-lua: types/wxl.d.lua from reflect.hpp and object_reads.def")
    add_dependencies(wxl-lua wxl-lua-types-gen)
else()
    # The superbuild: the LuaJIT notice goes next to mod.json, since the binaries carry it.
    file(READ mod.json _manifest)
    string(REGEX MATCH "\"id\"[ \t\r\n]*:[ \t\r\n]*\"([^\"]*)\"" _ "${_manifest}")
    set(_mod_dir "${CMAKE_BINARY_DIR}/dist/${CMAKE_MATCH_1}")
    add_custom_command(
        OUTPUT "${_mod_dir}/LICENSE-LuaJIT.txt"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
                "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE-LuaJIT.txt" "${_mod_dir}/LICENSE-LuaJIT.txt"
        DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE-LuaJIT.txt"
        COMMENT "wxl-lua: LICENSE-LuaJIT.txt -> ${_mod_dir}")
    add_custom_target(wxl-lua-notice ALL DEPENDS "${_mod_dir}/LICENSE-LuaJIT.txt")
endif()

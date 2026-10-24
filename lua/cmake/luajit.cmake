# SPDX-License-Identifier: MIT
# Copyright (c) 2026 WarcraftXL
#
# LuaJIT 2.1 as a static library for the architecture of this build tree.
#
# The source is fetched at a pinned commit and built by its own msvcbuild.bat, which compiles
# in place: the src/ tree is copied into the build directory first, so two trees (x64, x86)
# never build over each other. The compiler comes from vcvarsall.bat of the Visual Studio that
# CMake is using, for the architecture CMake is targeting. CL=/MD makes the objects use the
# dynamic CRT, which is what the mod is built with in every configuration.
#
# Provides the imported target luajit::lib and the variable WXL_LUAJIT_SOURCE_DIR.

include_guard(GLOBAL)

set(WXL_LUAJIT_COMMIT 04dca7911ea255f37be799c18d74c305b921c1a6 CACHE STRING
    "The LuaJIT commit wxl.lua is built with (v2.1, 2024-07-04)")

include(FetchContent)
FetchContent_Declare(luajit
    GIT_REPOSITORY https://github.com/LuaJIT/LuaJIT.git
    GIT_TAG        ${WXL_LUAJIT_COMMIT})
FetchContent_MakeAvailable(luajit)
set(WXL_LUAJIT_SOURCE_DIR "${luajit_SOURCE_DIR}")

# --- the build tree's own copy ---------------------------------------------------------------

set(_luajit_build "${CMAKE_CURRENT_BINARY_DIR}/luajit")
file(COPY "${luajit_SOURCE_DIR}/src" "${luajit_SOURCE_DIR}/dynasm" DESTINATION "${_luajit_build}")

# luajit.h carries the commit time as its rolling version; msvcbuild.bat reads it from
# ..\.relver when there is no .git beside src/.
execute_process(
    COMMAND git -C "${luajit_SOURCE_DIR}" show -s --format=%ct
    OUTPUT_VARIABLE _luajit_relver
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET)
if(NOT _luajit_relver)
    set(_luajit_relver "ROLLING")
endif()
file(WRITE "${_luajit_build}/.relver" "${_luajit_relver}\n")

# --- the compiler ----------------------------------------------------------------------------

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(_luajit_arch x64)
else()
    set(_luajit_arch x86)
endif()

set(_luajit_vcvars "")
if(CMAKE_GENERATOR_INSTANCE)
    string(REGEX REPLACE ",.*$" "" _vs_root "${CMAKE_GENERATOR_INSTANCE}")
    set(_luajit_vcvars "${_vs_root}/VC/Auxiliary/Build/vcvarsall.bat")
elseif(DEFINED ENV{VCINSTALLDIR})
    set(_luajit_vcvars "$ENV{VCINSTALLDIR}/Auxiliary/Build/vcvarsall.bat")
endif()
if(_luajit_vcvars AND NOT EXISTS "${_luajit_vcvars}")
    message(FATAL_ERROR "wxl.lua: vcvarsall.bat is not at ${_luajit_vcvars}; LuaJIT cannot be built")
endif()

file(TO_NATIVE_PATH "${_luajit_build}/src" _luajit_src_native)
file(TO_NATIVE_PATH "${_luajit_vcvars}" _luajit_vcvars_native)

# One script does the whole build, so the custom command has nothing to quote. The lines end
# in \n: file(WRITE) writes the platform's line ending for it.
set(_luajit_script "${_luajit_build}/build.cmd")
set(_luajit_script_text "@echo off\n")
if(_luajit_vcvars)
    string(APPEND _luajit_script_text
        "call \"${_luajit_vcvars_native}\" ${_luajit_arch} >nul\n"
        "if errorlevel 1 exit /b 1\n")
endif()
# msvcbuild.bat runs the tools it builds (minilua, buildvm) by bare name from its directory,
# which an environment that sets NoDefaultCurrentDirectoryInExePath would refuse.
string(APPEND _luajit_script_text
    "set CL=/MD\n"
    "set \"NoDefaultCurrentDirectoryInExePath=\"\n"
    "cd /d \"${_luajit_src_native}\"\n"
    "if exist lua51.lib del lua51.lib\n"
    "call .\\msvcbuild.bat static\n"
    "if not exist lua51.lib exit /b 1\n"
    "exit /b 0\n")
file(WRITE "${_luajit_script}" "${_luajit_script_text}")

# --- the library -----------------------------------------------------------------------------

set(_luajit_lib "${_luajit_build}/src/lua51.lib")
file(GLOB _luajit_inputs
    "${_luajit_build}/src/*.c" "${_luajit_build}/src/*.h" "${_luajit_build}/src/*.dasc"
    "${_luajit_build}/src/host/*.c" "${_luajit_build}/src/host/*.h")

add_custom_command(
    OUTPUT "${_luajit_lib}"
    COMMAND "${_luajit_script}"
    DEPENDS ${_luajit_inputs} "${_luajit_script}"
    COMMENT "wxl.lua: building LuaJIT (${_luajit_arch}, static, /MD)"
    VERBATIM)

add_custom_target(wxl-luajit DEPENDS "${_luajit_lib}")

add_library(luajit::lib STATIC IMPORTED GLOBAL)
set_target_properties(luajit::lib PROPERTIES
    IMPORTED_LOCATION "${_luajit_lib}"
    INTERFACE_INCLUDE_DIRECTORIES "${_luajit_build}/src")
add_dependencies(luajit::lib wxl-luajit)

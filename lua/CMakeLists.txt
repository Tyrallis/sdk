cmake_minimum_required(VERSION 3.21)

# wxl.lua: the Lua mod runtime, itself a mod built with wxl_add_mod. From this directory it is
# a superbuild producing dist/wxl.lua/ for both architectures; from the SDK's own tree, which
# sets WXL_MOD_ARCH, it is the library target of that tree's architecture. Configured on its own
# with WXL_MOD_ARCH set -- a child of the superbuild, or a runtime's build that ships it -- its
# install is the mod folder: mod.json, LICENSE-LuaJIT.txt and bin/<arch>/wxl-lua.dll.
project(wxl-lua VERSION 0.1.0 LANGUAGES NONE)

option(WXL_LUA_TYPES "Rewrite types/wxl.d.lua from the tables at each build. The tool is a \
Windows executable: a build that cannot run one (no emulator) or must not write into the SDK \
checkout turns this off" ON)

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
    TARGETS win-x64 win-x86
    CXX_STANDARD 17)

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
    if(WXL_LUA_TYPES)
        add_custom_target(wxl-lua-types-gen ALL
            COMMAND wxl-lua-types "${CMAKE_CURRENT_SOURCE_DIR}/types/wxl.d.lua"
            COMMENT "wxl-lua: types/wxl.d.lua from reflect.hpp and object_reads.def")
        add_dependencies(wxl-lua wxl-lua-types-gen)
    endif()

    # On its own, the install is the whole mod folder: the manifest, and the LuaJIT notice the
    # binaries carry, beside bin/<arch>/. Inside the SDK's tree there is no mod folder to fill.
    if(PROJECT_IS_TOP_LEVEL)
        install(FILES mod.json LICENSE-LuaJIT.txt DESTINATION .)
    endif()
endif()

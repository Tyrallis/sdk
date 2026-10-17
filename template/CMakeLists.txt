cmake_minimum_required(VERSION 3.21)

# No language here: wxl_add_mod enables C++ in each per-architecture child build.
project(wxl-example VERSION 1.0.0 LANGUAGES NONE)

# A checkout of WarcraftXL/sdk. With an installed SDK, replace these two lines by
# find_package(wxl-sdk REQUIRED).
set(WXL_SDK_DIR "${CMAKE_CURRENT_SOURCE_DIR}/.." CACHE PATH "Path to the WarcraftXL SDK")
include("${WXL_SDK_DIR}/cmake/wxl_add_mod.cmake")

# One source, one command, two binaries: dist/wxl.example/{mod.json,bin/x64,bin/x86}.
wxl_add_mod(example
    SOURCES src/mod.cpp
    MANIFEST mod.json
    TARGETS win-x64 win-x86)

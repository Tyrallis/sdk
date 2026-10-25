cmake_minimum_required(VERSION 3.21)

# No language here: wxl_add_mod enables C++ in each per-architecture build.
project(murloc-hat VERSION 1.0.0 LANGUAGES NONE)

# Where the SDK is. The default is this checkout; pass -DWXL_SDK_DIR=<path> for another one.
set(WXL_SDK_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../.." CACHE PATH "Path to the WarcraftXL SDK")
include("${WXL_SDK_DIR}/cmake/wxl_add_mod.cmake")

wxl_add_mod(murloc-hat
    SOURCES src/MurlocHat.cpp
    MANIFEST mod.json
    TARGETS win-x64 win-x86)

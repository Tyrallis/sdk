cmake_minimum_required(VERSION 3.21)

project(wxl-example VERSION 1.0.0 LANGUAGES CXX)

# Point this at a checkout of WarcraftXL/sdk, or install the SDK and use find_package(wxl-sdk).
set(WXL_SDK_DIR "${CMAKE_CURRENT_SOURCE_DIR}/.." CACHE PATH "Path to the WarcraftXL SDK")

if(NOT TARGET wxl::sdk)
    add_subdirectory("${WXL_SDK_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/wxl-sdk")
endif()

add_library(example MODULE src/mod.cpp)

# wxl::sdk-mod is wxl::sdk plus WXL_BUILDING_MOD, which declares the entry points.
target_link_libraries(example PRIVATE wxl::sdk-mod)

set_target_properties(example PROPERTIES
    OUTPUT_NAME "example"
    PREFIX ""
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON)

if(MSVC)
    target_compile_options(example PRIVATE /W4 /permissive-)
else()
    target_compile_options(example PRIVATE -Wall -Wextra)
endif()

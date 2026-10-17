cmake_minimum_required(VERSION 3.21)

project(wxl-sdk
    VERSION 0.1.0
    DESCRIPTION "The WarcraftXL mod contract"
    LANGUAGES CXX)

# Header-only: nothing here is compiled into the runtime or into a mod as a library. A mod links
# the interface target and gets the include path, the language level and the mod-build define.
add_library(wxl-sdk INTERFACE)
add_library(wxl::sdk ALIAS wxl-sdk)

target_include_directories(wxl-sdk INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>)

target_compile_features(wxl-sdk INTERFACE cxx_std_17)

# Declares the entry points in mod.h. A runtime includes the same headers without it, so that it
# does not export the symbols it goes looking for.
add_library(wxl-sdk-mod INTERFACE)
add_library(wxl::sdk-mod ALIAS wxl-sdk-mod)
target_link_libraries(wxl-sdk-mod INTERFACE wxl-sdk)
target_compile_definitions(wxl-sdk-mod INTERFACE WXL_BUILDING_MOD)

option(WXL_SDK_TESTS "Build the contract tests" ${PROJECT_IS_TOP_LEVEL})

add_subdirectory(runtime)

if(WXL_SDK_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

add_subdirectory(host)

install(TARGETS wxl-sdk wxl-sdk-mod EXPORT wxl-sdk-targets)
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(EXPORT wxl-sdk-targets
    FILE wxl-sdk-targets.cmake
    NAMESPACE wxl::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/wxl-sdk)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/wxl-sdk-config-version.cmake"
    COMPATIBILITY SameMajorVersion
    ARCH_INDEPENDENT)

file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/wxl-sdk-config.cmake"
    "include(\"\${CMAKE_CURRENT_LIST_DIR}/wxl-sdk-targets.cmake\")\n")

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/wxl-sdk-config.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/wxl-sdk-config-version.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/wxl-sdk)

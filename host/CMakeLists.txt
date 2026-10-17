# The shared mod host: what every runtime does the same way. Manifest, discovery, load order,
# admission, loading, services, events, stages, logging. Nothing client-specific lives here.
add_library(wxl-host STATIC
    src/admission.cpp
    src/discovery.cpp
    src/event_bus.cpp
    src/json.cpp
    src/loader.cpp
    src/log.cpp
    src/manifest.cpp
    src/mod_host.cpp
    src/platform.cpp
    src/registry.cpp)
add_library(wxl::host ALIAS wxl-host)

target_include_directories(wxl-host PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>)

target_link_libraries(wxl-host PUBLIC wxl::sdk)
target_compile_features(wxl-host PUBLIC cxx_std_17)

if(MSVC)
    # /EHs rather than /EHsc: a handler reached through a C function pointer may throw, and the
    # event bus catches that at the boundary. With 'c' the compiler would assume it cannot.
    string(REPLACE "/EHsc" "/EHs" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
    target_compile_options(wxl-host PRIVATE /W4 /permissive- /EHs)
else()
    target_compile_options(wxl-host PRIVATE -Wall -Wextra)
endif()

if(WXL_SDK_TESTS)
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../tests/host ${CMAKE_CURRENT_BINARY_DIR}/tests)
endif()

install(TARGETS wxl-host EXPORT wxl-sdk-targets)
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

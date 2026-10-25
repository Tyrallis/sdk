# The shared runtime scaffolding: everything a runtime needs that does not depend on how the
# client is reached. A runtime links this and supplies the bindings.
add_library(wxl-runtime STATIC
    src/handles.cpp
    src/object_service.cpp
    src/object_reads.cpp)
add_library(wxl::runtime ALIAS wxl-runtime)

target_include_directories(wxl-runtime PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>)

target_link_libraries(wxl-runtime PUBLIC wxl::sdk)
target_compile_features(wxl-runtime PUBLIC cxx_std_17)

if(MSVC)
    target_compile_options(wxl-runtime PRIVATE /W4 /permissive-)
else()
    target_compile_options(wxl-runtime PRIVATE -Wall -Wextra)
endif()

include(GNUInstallDirs)

install(TARGETS wxl-runtime EXPORT wxl-sdk-targets)
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

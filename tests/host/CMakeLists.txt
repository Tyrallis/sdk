# The mod host checks, and a real mod built from the SDK for the loader to open.
add_library(wxl-host-fixture MODULE fixture/mod.cpp)
target_link_libraries(wxl-host-fixture PRIVATE wxl::sdk-mod)
set_target_properties(wxl-host-fixture PROPERTIES OUTPUT_NAME "fixture" PREFIX "")

add_executable(wxl-host-tests host_tests.cpp)
target_link_libraries(wxl-host-tests PRIVATE wxl::host)

foreach(target wxl-host-fixture wxl-host-tests)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /permissive-)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra)
    endif()
endforeach()

add_dependencies(wxl-host-tests wxl-host-fixture)

# The test is told where the fixture binary is and where it may write a mods tree.
add_test(NAME host
    COMMAND wxl-host-tests $<TARGET_FILE:wxl-host-fixture> ${CMAKE_CURRENT_BINARY_DIR}/scratch)

# The conformance checks. A runtime passes them by building contract.cpp against its own host in
# place of fake_host.hpp.
add_executable(wxl-contract contract.cpp)
target_link_libraries(wxl-contract PRIVATE wxl::sdk)
target_include_directories(wxl-contract PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

if(MSVC)
    target_compile_options(wxl-contract PRIVATE /W4 /permissive-)
else()
    target_compile_options(wxl-contract PRIVATE -Wall -Wextra)
endif()

add_test(NAME contract COMMAND wxl-contract)

# The shared runtime scaffolding, against a binding backed by a table instead of a client.
add_executable(wxl-runtime-tests runtime.cpp)
target_link_libraries(wxl-runtime-tests PRIVATE wxl::runtime)

if(MSVC)
    target_compile_options(wxl-runtime-tests PRIVATE /W4 /permissive-)
else()
    target_compile_options(wxl-runtime-tests PRIVATE -Wall -Wextra)
endif()

add_test(NAME runtime COMMAND wxl-runtime-tests)

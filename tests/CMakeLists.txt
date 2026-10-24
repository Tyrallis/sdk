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

# The field table against the args structs it describes.
add_executable(wxl-reflect reflect.cpp)
target_link_libraries(wxl-reflect PRIVATE wxl::sdk)

if(MSVC)
    target_compile_options(wxl-reflect PRIVATE /W4 /permissive-)
else()
    target_compile_options(wxl-reflect PRIVATE -Wall -Wextra)
endif()

add_test(NAME reflect COMMAND wxl-reflect)

# The shared runtime scaffolding, against a binding backed by a table instead of a client.
add_executable(wxl-runtime-tests runtime.cpp)
target_link_libraries(wxl-runtime-tests PRIVATE wxl::runtime)

if(MSVC)
    target_compile_options(wxl-runtime-tests PRIVATE /W4 /permissive-)
else()
    target_compile_options(wxl-runtime-tests PRIVATE -Wall -Wextra)
endif()

add_test(NAME runtime COMMAND wxl-runtime-tests)

# wxl.lua against the mod host: Lua mods written into a scratch tree, the lua-hello example,
# the events raised from both threads, a handler that errors, a reload.
if(TARGET wxl-lua)
    add_executable(wxl-lua-tests lua_tests.cpp)
    target_link_libraries(wxl-lua-tests PRIVATE wxl::host)
    target_include_directories(wxl-lua-tests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/../lua/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../runtime/include)
    add_dependencies(wxl-lua-tests wxl-lua)

    if(MSVC)
        target_compile_options(wxl-lua-tests PRIVATE /W4 /permissive-)
    else()
        target_compile_options(wxl-lua-tests PRIVATE -Wall -Wextra)
    endif()

    add_test(NAME lua
        COMMAND wxl-lua-tests $<TARGET_FILE:wxl-lua>
                ${CMAKE_CURRENT_SOURCE_DIR}/../examples/lua-hello
                ${CMAKE_CURRENT_BINARY_DIR}/lua-scratch)

    # The committed editor definitions against what the tables say now.
    add_test(NAME lua-types
        COMMAND wxl-lua-types --check ${CMAKE_CURRENT_SOURCE_DIR}/../lua/types/wxl.d.lua)
endif()

# The documents against the example they quote. A tutorial whose snippets have drifted still
# builds and still reads well; only this says it is wrong.
find_package(Python3 COMPONENTS Interpreter QUIET)
if(Python3_Interpreter_FOUND)
    add_test(NAME docs COMMAND "${Python3_EXECUTABLE}" -I
             "${CMAKE_CURRENT_SOURCE_DIR}/../examples/check_docs.py")
else()
    message(WARNING "Python 3 was not found, so the documentation check will not run. "
        "Run sdk/examples/check_docs.py by hand.")
endif()

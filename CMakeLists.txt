cmake_minimum_required(VERSION 3.10)
project(TelnetServer)

set(CMAKE_CXX_STANDARD 17)

if (MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Werror)
endif()

add_executable(telnet_server
    main.cpp
    tlnt.cpp
    parser.cpp
    gc.cpp
)
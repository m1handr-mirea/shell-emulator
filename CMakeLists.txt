cmake_minimum_required(VERSION 3.15)
project(shell_emulator CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic")

include_directories(src)

file(GLOB SOURCES "src/*.cpp")

add_executable(shell_emulator ${SOURCES})

cmake_minimum_required(VERSION 3.15)
project(untitled)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_FLAGS -pthread)

add_executable(untitled main.cpp Command.h Utils.h Command.cpp Interpreter.h Interpreter.cpp Parser.h Utils.cpp Parser.cpp)
cmake_minimum_required(VERSION 3.0.0)
project(particles VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 20)

add_executable(particles main.cpp)

target_link_libraries(particles pthread)

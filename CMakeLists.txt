cmake_minimum_required(VERSION 3.0.0)
project(particles VERSION 0.1.0)

add_executable(particles main.cpp)

set_target_properties(particles PROPERTIES CXX_STANDARD 17)

target_link_libraries(particles pthread)

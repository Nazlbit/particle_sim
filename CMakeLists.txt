cmake_minimum_required(VERSION 3.0.0)
project(particles VERSION 0.1.0)

add_executable(particles main.cpp math.hpp math.cpp simulation.hpp simulation.cpp barrier.hpp exception.hpp helper.hpp application.hpp application.cpp)

set_target_properties(particles PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON)

target_link_libraries(particles pthread)

cmake_minimum_required(VERSION 3.0.0)
project(particles VERSION 0.1.0)

set(SRC_FILES main.cpp math.cpp simulation.cpp application.cpp)
set(HEADER_FILES application.hpp barrier.hpp exception.hpp glfw_singleton.hpp helper.hpp math.hpp simulation.hpp)

add_executable(particles ${SRC_FILES} ${HEADER_FILES})

set_target_properties(particles PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON)

target_link_libraries(particles pthread)

add_subdirectory(dependencies/glad)

target_link_libraries(particles glad)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

add_subdirectory(dependencies/glfw)

target_link_libraries(particles glfw)

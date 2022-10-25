cmake_minimum_required(VERSION 3.0.0)
project(glad VERSION 1.0)

add_library(glad src/gl.c)

target_include_directories(glad PUBLIC include)

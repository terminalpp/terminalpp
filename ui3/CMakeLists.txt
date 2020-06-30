# CMakeList.txt : CMake project for t++, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.5)

project(libui3)

file(GLOB_RECURSE SRC "*.cpp" "*.h")

add_library(libui3 ${SRC})

# CMakeList.txt : CMake project for t++, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.5)

project(tserv)

include_directories(".")

file(GLOB_RECURSE SRC "*.cpp" "*.h")

add_executable(tserv ${SRC})

target_link_libraries(tserv libvterm)

# CMakeList.txt : CMake project for t++, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.5)

project(libuiterminal)

file(GLOB_RECURSE SRC "*.cpp" "*.h")

if(WIN32)
    add_library(libuiterminal ${SRC})
    target_link_libraries(libuiterminal libtpp)
elseif(UNIX)
	find_library(LUTIL util)
    add_library(libuiterminal ${SRC})
    target_link_libraries(libuiterminal PUBLIC ${LUTIL} libtpp)
else()
    message(FATAL_ERROR "Only Windows and Linux are supported for now")
endif()

# CMakeList.txt : CMake project for t++, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.5)

project(libtpp)

file(GLOB_RECURSE SRC "*.cpp" "*.h")

if(WIN32)
    add_library(libtpp ${SRC})
elseif(UNIX)
	#find_library(LUTIL util)
    add_library(libtpp ${SRC})
	#target_link_libraries(libtpp PUBLIC ${LUTIL})
else()
    message(FATAL_ERROR "Only Windows and Linux are supported for now")
endif()

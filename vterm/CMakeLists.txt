# CMakeList.txt : CMake project for t++, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.5)

project(libvterm)

file(GLOB_RECURSE SRC "*.cpp" "*.h")

if (UNIX)
	add_definitions(
			-std=c++11 -g -Wall
		)
endif (UNIX)


add_library(libvterm ${SRC})

# CMakeList.txt : CMake project for t++, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.5)

project(libvterm)

file(GLOB_RECURSE SRC "*.cpp" "*.h")

if(WIN32)
    add_library(libvterm ${SRC})
elseif(UNIX)
	find_library(LUTIL util)
    add_library(libvterm ${SRC})
	target_link_libraries(libvterm PUBLIC ${LUTIL})
else()
    message(FATAL_ERROR "Only Windows and Linux are supported for now")
endif()

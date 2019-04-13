# CMakeList.txt : CMake project for t++, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.5)

project(tpp)

include_directories(".")

file(GLOB_RECURSE SRC "*.cpp" "*.h")

if (WIN32)
    add_compile_definitions(TPP_RENDERED=gdi)
    add_executable(tpp WIN32 ${SRC})
else (WIN32)
       find_package(X11 REQUIRED)
	   include_directories(${X11_INCLUDE_DIR})
       link_libraries(${X11_LIBRARIES})  
       add_definitions(
			-std=c++11 -g -Wall
		)
    add_executable(tpp ${SRC})
endif (WIN32)

target_link_libraries(tpp libvterm)

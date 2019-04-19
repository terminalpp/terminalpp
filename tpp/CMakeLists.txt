# CMakeList.txt : CMake project for t++, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.5)

project(tpp)

include_directories(".")

file(GLOB_RECURSE SRC "*.cpp" "*.h")

if(WIN32)
    add_compile_definitions(TPP_RENDERED=gdi)
    add_executable(tpp WIN32 ${SRC})
elseif(UNIX)
    find_package(X11 REQUIRED)
	find_package(Threads REQUIRED)
    find_package(Freetype REQUIRED)
	message(${LUTIL})
    include_directories(${X11_INCLUDE_DIR})
    include_directories(${FREETYPE_INCLUDE_DIRS})
    add_definitions(
	    -std=c++11 -g -Wall
	)
	if (NOT X11_Xft_FOUND)
	    message(FATAL_ERROR "xft not found - please install libxft-dev")
	endif()
    add_executable(tpp ${SRC})
    target_link_libraries(tpp ${X11_LIBRARIES} ${X11_Xft_LIB})  
    target_link_libraries(tpp ${X11_Xft_LIB})  
	target_link_libraries(tpp ${CMAKE_THREAD_LIBS_INIT})
    target_link_libraries(tpp ${FREETYPE_LIBRARIES})
else()
    message(FATAL_ERROR "Only Windows and Linux are supported for now")
endif() 

target_link_libraries(tpp libvterm)

cmake_minimum_required (VERSION 3.5)

project(asciienc)

#if(UNIX)
	find_package(Threads REQUIRED)
#endif


include_directories(".")

file(GLOB_RECURSE SRC "*.cpp" "*.h")

add_executable(asciienc ${SRC})

target_link_libraries(asciienc libvterm)

#if(UNIX)
	target_link_libraries(asciienc ${CMAKE_THREAD_LIBS_INIT})
#endif

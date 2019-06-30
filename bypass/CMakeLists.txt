cmake_minimum_required (VERSION 3.5)

project(bypass)

#if(UNIX)
	find_package(Threads REQUIRED)
#endif


include_directories(".")

file(GLOB_RECURSE SRC "*.cpp" "*.h")

add_executable(bypass ${SRC})

target_link_libraries(bypass libvterm)

#if(UNIX)
	target_link_libraries(bypass ${CMAKE_THREAD_LIBS_INIT})
#endif

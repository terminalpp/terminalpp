cmake_minimum_required (VERSION 3.5)

if(UNIX AND NOT APPLE) 
    project(bypass)
	find_package(Threads REQUIRED)

    include_directories(".")

    file(GLOB_RECURSE SRC "*.cpp" "*.h")

	add_executable(bypass ${SRC})

	target_link_libraries(bypass libvterm)

	target_link_libraries(bypass ${CMAKE_THREAD_LIBS_INIT})
endif()
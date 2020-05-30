cmake_minimum_required (VERSION 3.5)

if(ARCH_LINUX) 
    project(tpp-bypass)
    add_compile_options(-std=c++17 -Wall -Wextra -pedantic)
    find_package(Threads REQUIRED)
    find_library(LUTIL util)
    file(GLOB_RECURSE SRC "main_bypass.cpp")
    add_executable(tpp-bypass ${SRC})
    target_link_libraries(tpp-bypass ${CMAKE_THREAD_LIBS_INIT} ${LUTIL})
    add_dependencies(tpp-bypass stamp)

    if(INSTALL STREQUAL tpp-bypass)
        install(TARGETS tpp-bypass DESTINATION bin COMPONENT tpp-bypass)
    endif()

endif()
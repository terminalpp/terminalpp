cmake_minimum_required (VERSION 3.5)

project(ropen VERSION ${TPP_VERSION})

# ropen only works on linux for now
if(ARCH_UNIX) 
    find_package(Threads REQUIRED)
    find_library(LUTIL util)
    file(GLOB_RECURSE SRC "*.cpp" "*.h")
    add_executable(ropen ${SRC})
    target_link_libraries(ropen libtpp ${CMAKE_THREAD_LIBS_INIT} ${LUTIL})
    add_dependencies(ropen stamp)
    if(NOT ARCH_MACOS)
        target_link_libraries(ropen stdc++fs)    
    endif()
endif()

if(INSTALL STREQUAL ropen AND ARCH_UNIX)
    install(TARGETS ropen DESTINATION bin COMPONENT ropen)
endif()
cmake_minimum_required (VERSION 3.5)

project(tpp-server VERSION ${TPP_VERSION})

# tpp-server only works on linux for now
if(ARCH_UNIX) 
    add_compile_options(-std=c++17 -Wall -Wextra -pedantic)
    find_package(Threads REQUIRED)
    find_library(LUTIL util)
    file(GLOB_RECURSE SRC "*.cpp" "*.h")
    add_executable(tpp-server ${SRC})
    target_link_libraries(tpp-server libui libtpp ${CMAKE_THREAD_LIBS_INIT} ${LUTIL})
    add_dependencies(tpp-server stamp)
    if(NOT ARCH_MACOS)
        target_link_libraries(tpp-server stdc++fs)    
    endif()
endif()

if(INSTALL STREQUAL tpp-server AND ARCH_UNIX)
    install(TARGETS tpp-server DESTINATION bin COMPONENT tpp-server)
endif()
cmake_minimum_required (VERSION 3.5)

project(tpp-server VERSION ${TPP_VERSION})

# tpp-server only works on linux for now
#if(ARCH_UNIX) 
    find_package(Threads REQUIRED)
    file(GLOB_RECURSE SRC "*.cpp" "*.h")
    add_executable(tpp-server ${SRC})
    target_link_libraries(tpp-server libuiterminal libui libtpp ${CMAKE_THREAD_LIBS_INIT})
    if(ARCH_UNIX)
        find_library(LUTIL util)
        target_link_libraries(tpp-server ${LUTIL})
        if(NOT ARCH_MACOS)
            target_link_libraries(tpp-server stdc++fs)    
        endif()
    endif()
    add_dependencies(tpp-server stamp)
#endif()

if(INSTALL STREQUAL tpp-server AND ARCH_UNIX)
    install(TARGETS tpp-server DESTINATION bin COMPONENT tpp-server)
endif()
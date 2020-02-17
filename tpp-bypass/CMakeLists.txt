cmake_minimum_required (VERSION 3.5)

if(ARCH_LINUX) 
    project(tpp-bypass)
    add_compile_options(-std=c++17 -Wall -Wextra -pedantic)
    find_package(Threads REQUIRED)
    find_library(LUTIL util)
    file(GLOB_RECURSE SRC "main_bypass.cpp")
    add_executable(tpp-bypass ${SRC})
    target_link_libraries(tpp-bypass ${CMAKE_THREAD_LIBS_INIT} ${LUTIL})

    if(PACKAGE_BYPASS)
        install(TARGETS tpp-bypass DESTINATION bin COMPONENT tpp-bypass)
        # cpack settings - generic
        set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "ConPTY Bypass." PARENT_SCOPE)
        set(CPACK_PACKAGE_DESCRIPTION "Bypasses terminal IO to standard input and output to bypass the ConPTY on Windows when WSL is used. " PARENT_SCOPE)
        set(CPACK_DEBIAN_PACKAGE_DEPENDS "" PARENT_SCOPE)
        set(CPACK_RPM_PACKAGE_REQUIRES "" PARENT_SCOPE)
        set(CPACK_PACKAGE_DEBIAN_SECTION "utils" PARENT_SCOPE)
    endif()

endif()
cmake_minimum_required (VERSION 3.5)

project(ropen VERSION ${TPP_VERSION})

# TODO this should work for mac too

if(ARCH_UNIX) 
    
    add_compile_options(-std=c++17 -Wall -Wextra -pedantic)
    #find_package(Threads REQUIRED)
    #find_library(LUTIL util)
    file(GLOB_RECURSE SRC "*.cpp" "*.h")
    add_executable(ropen ${SRC})
    target_link_libraries(ropen libtpp)
    if(NOT ARCH_MACOS)
        target_link_libraries(ropen stdc++fs)    
    endif()
    
    if(PACKAGE_ROPEN)
        install(TARGETS ropen DESTINATION bin COMPONENT ropen)
        # cpack settings - generic
        set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Remote file opener.")
        set(CPACK_PACKAGE_DESCRIPTION "Allows terminal++ sessions to open remote files and view them locally by transmitting the file in the existing terminal connection.")
        # we need to propagate these to the parent scope *and* have them in the local scope because the snap target below might need it
        set(CPACK_PACKAGE_DESCRIPTION_SUMMARY ${CPACK_PACKAGE_DESCRIPTION_SUMMARY}  PARENT_SCOPE)
        set(CPACK_PACKAGE_DESCRIPTION ${CPACK_PACKAGE_DESCRIPTION}  PARENT_SCOPE)
        # set rpm and dep options
        set(CPACK_DEBIAN_PACKAGE_DEPENDS "" PARENT_SCOPE)
        set(CPACK_RPM_PACKAGE_REQUIRES "" PARENT_SCOPE)
        set(CPACK_PACKAGE_DEBIAN_SECTION "utils" PARENT_SCOPE)
    endif()

    # Builds the snap package for linux
    #
    if (PACKAGE_ROPEN AND PACKAGE_SNAP)
        # create the build directory and configure the WiX configuration in it
        configure_file(${CMAKE_CURRENT_SOURCE_DIR}/packages/snap/ropen.yaml ${CMAKE_CURRENT_BINARY_DIR}/snap/snapcraft.yaml @ONLY)
        add_custom_target(package-snap-copy-ropen
            COMMAND rm -rf ${CMAKE_CURRENT_BINARY_DIR}/snap/source
            COMMAND mkdir ${CMAKE_CURRENT_BINARY_DIR}/snap/source
            COMMAND rsync -a . ${CMAKE_CURRENT_BINARY_DIR}/snap/source  --exclude-from=.gitignore
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        )
        set_target_properties(package-snap-copy-ropen PROPERTIES EXCLUDE_FROM_ALL TRUE)
        add_custom_target(package-snap
            COMMAND ${PACKAGE_SNAP} clean ${SNAP_EXTRA_ARGS}
            COMMAND ${PACKAGE_SNAP} ${SNAP_EXTRA_ARGS}
            COMMAND cp ${PROJECT_NAME}_${PROJECT_VERSION}_amd64.snap ${CMAKE_BINARY_DIR}/packages/${PROJECT_NAME}.snap
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/snap
            DEPENDS package-snap-copy-ropen
        )
    endif()

endif()

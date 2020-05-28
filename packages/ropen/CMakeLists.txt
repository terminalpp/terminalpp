set(PROEJCT_VENDOR "zduka")

#
if(PACKAGE_SNAP AND INSTALL STREQUAL ropen)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/snap/ropen.yaml ${CMAKE_CURRENT_BINARY_DIR}/snap/snapcraft.yaml @ONLY)
    add_custom_target(ropen-snap-copy-ropen
        COMMAND rm -rf ${CMAKE_CURRENT_BINARY_DIR}/snap/source
        COMMAND mkdir ${CMAKE_CURRENT_BINARY_DIR}/snap/source
        COMMAND rsync -a . ${CMAKE_CURRENT_BINARY_DIR}/snap/source  --exclude-from=.gitignore
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    )
    set_target_properties(ropen-snap-copy-ropen PROPERTIES EXCLUDE_FROM_ALL TRUE)
    add_custom_target(ropen-snap
        COMMAND ${PACKAGE_SNAP} clean ${SNAP_EXTRA_ARGS}
        COMMAND ${PACKAGE_SNAP} ${SNAP_EXTRA_ARGS}
        COMMAND cp ropen_${PROJECT_VERSION}_amd64.snap ${CMAKE_BINARY_DIR}/packages/ropen.snap
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/snap
        DEPENDS ropen-snap-copy-ropen
    )
    add_dependencies(packages ropen-snap)
endif()
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

# Debian source packages
#
if(PACKAGE_DEB)
    # configure the debian files 
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/deb/debian/control ${CMAKE_CURRENT_BINARY_DIR}/configured/deb/control @ONLY)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/deb/debian/changelog ${CMAKE_CURRENT_BINARY_DIR}/configured/deb/changelog @ONLY)

    add_custom_target(ropen-deb-prepare
        # first clear the output folder
        COMMAND rm -rf ${CMAKE_CURRENT_BINARY_DIR}/deb/ropen
        COMMAND mkdir -p ${CMAKE_CURRENT_BINARY_DIR}/deb/ropen
        # copy the terminalpp sources
        COMMAND rsync -a . ${CMAKE_CURRENT_BINARY_DIR}/deb/ropen --exclude-from=.gitignore --exclude ".git"
        # copy the stamp, the debian folder to the terminalpp root and the configured files
        COMMAND cp stamp.h ${CMAKE_CURRENT_BINARY_DIR}/deb/ropen/stamp.h
        COMMAND cp -r ${CMAKE_CURRENT_SOURCE_DIR}/deb/debian ${CMAKE_CURRENT_BINARY_DIR}/deb/ropen
        COMMAND cp -f ${CMAKE_CURRENT_BINARY_DIR}/configured/deb/* ${CMAKE_CURRENT_BINARY_DIR}/deb/ropen/debian
        # copy the makefile
        COMMAND cp ${CMAKE_CURRENT_SOURCE_DIR}/deb/Makefile ${CMAKE_CURRENT_BINARY_DIR}/deb/ropen
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        DEPENDS stamp
    )
    add_custom_target(ropen-deb-src
        # now we are ready to build the package
        COMMAND debuild -S -sa -us -uc
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/deb/ropen
        DEPENDS ropen-deb-prepare
    )
    add_dependencies(packages ropen-deb-src)

    add_custom_target(ropen-deb
        # now we are ready to build the package
        COMMAND debuild -b -us -uc
        # and copy the result to the packages folder
        COMMAND cp ${CMAKE_CURRENT_BINARY_DIR}/deb/ropen_${PROJECT_VERSION}_amd64.deb ${CMAKE_BINARY_DIR}/packages/ropen.deb
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/deb/ropen
        DEPENDS ropen-deb-prepare
    )
    add_dependencies(packages ropen-deb)
endif()

if(PACKAGE_RPM)
    # configure the spec file
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/rpm/ropen.spec ${CMAKE_CURRENT_BINARY_DIR}/configured/rpm/ropen.spec @ONLY)

    add_custom_target(ropen-rpm-prepare
        # first clear the output folder and prepare the required directories
        COMMAND rm -rf ${CMAKE_CURRENT_BINARY_DIR}/rpm
        COMMAND mkdir -p ${CMAKE_CURRENT_BINARY_DIR}/rpm/SOURCES
        COMMAND mkdir -p ${CMAKE_CURRENT_BINARY_DIR}/rpm/SPECS
        COMMAND mkdir -p ${CMAKE_CURRENT_BINARY_DIR}/rpm/BUILD
        COMMAND mkdir -p ${CMAKE_CURRENT_BINARY_DIR}/rpm/RPMS
        COMMAND mkdir -p ${CMAKE_CURRENT_BINARY_DIR}/rpm/SRPMS
        # copy the source of terminalpp to terminalpp directory and add the stamp
        COMMAND rsync -a . ${CMAKE_CURRENT_BINARY_DIR}/rpm/ropen --exclude-from=.gitignore --exclude ".git"
        COMMAND cp stamp.h ${CMAKE_CURRENT_BINARY_DIR}/rpm/ropen/stamp.h
        # compress the sources and delete the uncompressed ones
        COMMAND tar -czvf ${CMAKE_CURRENT_BINARY_DIR}/rpm/SOURCES/ropen.tar.gz -C ${CMAKE_CURRENT_BINARY_DIR}/rpm ropen
        COMMAND rm -rf ${CMAKE_CURRENT_BINARY_DIR}/rpm/ropen
        # copy the configured spec file
        COMMAND cp ${CMAKE_CURRENT_BINARY_DIR}/configured/rpm/ropen.spec ${CMAKE_CURRENT_BINARY_DIR}/rpm/SPECS/ropen.spec
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        DEPENDS stamp
    )

    add_custom_target(ropen-rpm
        COMMAND rpmbuild --define "_topdir ${CMAKE_CURRENT_BINARY_DIR}/rpm" -ba ropen.spec
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/rpm/SPECS
        COMMAND cp ${CMAKE_CURRENT_BINARY_DIR}/rpm/SRPMS/ropen-${PROJECT_VERSION}-1.src.rpm ${CMAKE_BINARY_DIR}/packages/ropen.src.rpm
        COMMAND cp ${CMAKE_CURRENT_BINARY_DIR}/rpm/RPMS/x86_64/ropen-${PROJECT_VERSION}-1.x86_64.rpm ${CMAKE_BINARY_DIR}/packages/ropen.rpm
        DEPENDS ropen-rpm-prepare
    )

    add_dependencies(packages ropen-rpm)

endif()

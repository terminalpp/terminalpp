set(PROJECT_EXECUTABLE "$<TARGET_FILE:terminalpp>")

# Windows
#
# on windows, the WIX toolset is used to produce the msi installer package. The package-msi target is defined here which runs WIX for terminalpp
if(PACKAGE_MSI AND RENDERER_NATIVE)
    # create the build directory and configure the WiX configuration in it
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/msi/terminalpp.wxs ${CMAKE_CURRENT_BINARY_DIR}/msi/terminalpp.wxs @ONLY)
    add_custom_target(terminalpp-msi
        COMMAND ${CMAKE_COMMAND} -E copy ${PROJECT_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/msi/terminalpp.exe
        COMMAND ${PACKAGE_MSI}/candle.exe
            -nologo 
            -arch "x64" 
            -ext WixUIExtension -ext WixUtilExtension 
            -out "terminalpp.wixobj" 
            "terminalpp.wxs"
        COMMAND ${PACKAGE_MSI}/light.exe 
            -nologo
            -ext WixUIExtension
            -ext WixUtilExtension 
            -out "${CMAKE_BINARY_DIR}/packages/terminalpp.msi" 
            -sice:ICE61 -sice:ICE91 
            "terminalpp.wixobj"
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/msi
        DEPENDS terminalpp
    )        
    add_dependencies(packages terminalpp-msi)
endif()

# generate the MSIX package
# https://www.advancedinstaller.com/msix-introduction.html
# https://docs.microsoft.com/en-us/windows/msix/package/create-app-package-with-makeappx-tool
if(PACKAGE_MSIX AND RENDERER_NATIVE)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/msix/appxmanifest.xml ${CMAKE_CURRENT_BINARY_DIR}/msix/appxmanifest.xml @ONLY)
    add_custom_target(terminalpp-msix
        COMMAND ${CMAKE_COMMAND} -E make_directory Assets
        COMMAND ${CMAKE_COMMAND} -E make_directory VFS
        COMMAND ${CMAKE_COMMAND} -E copy
                ${PROJECT_EXECUTABLE}
                VFS/terminalpp.exe
        COMMAND ${CMAKE_COMMAND} -E copy
                ${CMAKE_SOURCE_DIR}/resources/icons/icon_44x44.png
                Assets/icons/icon_44x44.png
        COMMAND ${CMAKE_COMMAND} -E copy
                ${CMAKE_SOURCE_DIR}/resources/icons/icon_44x44.png
                Assets/icons/icon_44x44.targetsize-44_altform-unplated.png
        COMMAND ${CMAKE_COMMAND} -E copy
                ${CMAKE_SOURCE_DIR}/resources/icons/icon_150x150.png
                Assets/icons/icon_150x150.png
        COMMAND ${CMAKE_COMMAND} -E copy
                ${CMAKE_SOURCE_DIR}/resources/icons/icon_150x150.png
                Assets/icons/icon_150x150.targetsize-150_altform-unplated.png
        COMMAND ${CMAKE_COMMAND} -E copy
                ${CMAKE_SOURCE_DIR}/resources/icons/icon_256x256.png
                Assets/icons/icon_256x256.png
        COMMAND ${CMAKE_COMMAND} -E copy
                ${CMAKE_SOURCE_DIR}/resources/icons/icon_256x256.png
                Assets/icons/icon_256x256.targetsize-256_altform-unplated.png
        COMMAND makepri.exe new /pr . /cf ${CMAKE_CURRENT_SOURCE_DIR}/msix/priconfig.xml /o
        COMMAND MakeAppx.exe pack /d . /p ${CMAKE_BINARY_DIR}/packages/terminalpp.msix /o
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/msix
        DEPENDS terminalpp
    )
    add_dependencies(packages terminalpp-msix)
    
    # also configure the preview build
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/msix/appxmanifest-preview.xml ${CMAKE_CURRENT_BINARY_DIR}/msix-preview/appxmanifest.xml @ONLY)
    add_custom_target(terminalpp-msix-preview
        COMMAND ${CMAKE_COMMAND} -E make_directory Assets
        COMMAND ${CMAKE_COMMAND} -E make_directory VFS
        COMMAND ${CMAKE_COMMAND} -E copy
                ${PROJECT_EXECUTABLE}
                VFS/terminalpp.exe
        COMMAND ${CMAKE_COMMAND} -E copy
                ${CMAKE_SOURCE_DIR}/resources/icons/icon_44x44.png
                Assets/icons/icon_44x44.png
        COMMAND ${CMAKE_COMMAND} -E copy
                ${CMAKE_SOURCE_DIR}/resources/icons/icon_44x44.png
                Assets/icons/icon_44x44.targetsize-44_altform-unplated.png
        COMMAND ${CMAKE_COMMAND} -E copy
                ${CMAKE_SOURCE_DIR}/resources/icons/icon_150x150.png
                Assets/icons/icon_150x150.png
        COMMAND ${CMAKE_COMMAND} -E copy
                ${CMAKE_SOURCE_DIR}/resources/icons/icon_150x150.png
                Assets/icons/icon_150x150.targetsize-150_altform-unplated.png
        COMMAND ${CMAKE_COMMAND} -E copy
                ${CMAKE_SOURCE_DIR}/resources/icons/icon_256x256.png
                Assets/icons/icon_256x256.png
        COMMAND ${CMAKE_COMMAND} -E copy
                ${CMAKE_SOURCE_DIR}/resources/icons/icon_256x256.png
                Assets/icons/icon_256x256.targetsize-256_altform-unplated.png
        COMMAND makepri.exe new /pr . /cf ${CMAKE_CURRENT_SOURCE_DIR}/msix/priconfig.xml /o
        COMMAND MakeAppx.exe pack /d . /p ${CMAKE_BINARY_DIR}/packages/terminalpp-preview.msix /o
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/msix-preview
        DEPENDS terminalpp
                
    )
    # don't build by default
    #add_dependencies(packages terminalpp-msix-preview)
endif()

# Linux
# =====
#
# Builds the snap package for linux
# The classic confinement is used as terminal with the strict confinement is most likely not what we want.
if (PACKAGE_SNAP AND RENDERER_NATIVE AND INSTALL STREQUAL terminalpp)
    # create the build directory and configure the snap configuration in it
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/snap/terminalpp.yaml ${CMAKE_CURRENT_BINARY_DIR}/snap/snapcraft.yaml @ONLY)
    add_custom_target(terminalpp-snap-copy-tpp
        COMMAND rm -rf ${CMAKE_CURRENT_BINARY_DIR}/snap/source
        COMMAND mkdir ${CMAKE_CURRENT_BINARY_DIR}/snap/source
        COMMAND rsync -a . ${CMAKE_CURRENT_BINARY_DIR}/snap/source  --exclude-from=.gitignore
        COMMAND git -C ${CMAKE_CURRENT_BINARY_DIR}/snap/source status
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    )
    set_target_properties(terminalpp-snap-copy-tpp PROPERTIES EXCLUDE_FROM_ALL TRUE)
    add_custom_target(terminalpp-snap
        COMMAND ${PACKAGE_SNAP} clean ${SNAP_EXTRA_ARGS}
        COMMAND ${PACKAGE_SNAP} ${SNAP_EXTRA_ARGS}
        COMMAND cp terminalpp_${PROJECT_VERSION}_amd64.snap ${CMAKE_BINARY_DIR}/packages/terminalpp.snap
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/snap
        DEPENDS terminalpp-snap-copy-tpp
    )
    add_dependencies(packages terminalpp-snap)
endif()

# Debian source packages
#
if(PACKAGE_DEB)
    # configure the debian files 
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/deb/debian/control ${CMAKE_CURRENT_BINARY_DIR}/configured/deb/control @ONLY)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/deb/debian/changelog ${CMAKE_CURRENT_BINARY_DIR}/configured/deb/changelog @ONLY)

    add_custom_target(terminalpp-deb-prepare
        # first clear the output folder
        COMMAND rm -rf ${CMAKE_CURRENT_BINARY_DIR}/deb/terminalpp
        COMMAND mkdir -p ${CMAKE_CURRENT_BINARY_DIR}/deb/terminalpp
        # copy the terminalpp sources
        COMMAND rsync -a . ${CMAKE_CURRENT_BINARY_DIR}/deb/terminalpp --exclude-from=.gitignore --exclude ".git"
        # copy the stamp, the debian folder to the terminalpp root and the configured files and the makefile
        COMMAND cp stamp.h ${CMAKE_CURRENT_BINARY_DIR}/deb/terminalpp/stamp.h
        COMMAND cp -r ${CMAKE_CURRENT_SOURCE_DIR}/deb/debian ${CMAKE_CURRENT_BINARY_DIR}/deb/terminalpp
        COMMAND cp -f ${CMAKE_CURRENT_BINARY_DIR}/configured/deb/* ${CMAKE_CURRENT_BINARY_DIR}/deb/terminalpp/debian
        COMMAND cp ${CMAKE_CURRENT_SOURCE_DIR}/deb/Makefile ${CMAKE_CURRENT_BINARY_DIR}/deb/terminalpp
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        DEPENDS stamp
    )
    add_custom_target(terminalpp-deb-src
        # now we are ready to build the package
        COMMAND debuild -S -sa -us -uc
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/deb/terminalpp
        DEPENDS terminalpp-deb-prepare
    )
    add_dependencies(packages terminalpp-deb-src)

    add_custom_target(terminalpp-deb
        # now we are ready to build the package
        COMMAND debuild -b -us -uc
        # and copy the result to the packages folder
        COMMAND cp ${CMAKE_CURRENT_BINARY_DIR}/deb/terminalpp_${PROJECT_VERSION}_amd64.deb ${CMAKE_BINARY_DIR}/packages/terminalpp.deb
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/deb/terminalpp
        DEPENDS terminalpp-deb-prepare
    )
    add_dependencies(packages terminalpp-deb)
endif()

if(PACKAGE_RPM)
    # configure the spec file
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/rpm/terminalpp.spec ${CMAKE_CURRENT_BINARY_DIR}/configured/rpm/terminalpp.spec @ONLY)

    add_custom_target(terminalpp-rpm-prepare
        # first clear the output folder and prepare the required directories
        COMMAND rm -rf ${CMAKE_CURRENT_BINARY_DIR}/rpm
        COMMAND mkdir -p ${CMAKE_CURRENT_BINARY_DIR}/rpm/SOURCES
        COMMAND mkdir -p ${CMAKE_CURRENT_BINARY_DIR}/rpm/SPECS
        COMMAND mkdir -p ${CMAKE_CURRENT_BINARY_DIR}/rpm/BUILD
        COMMAND mkdir -p ${CMAKE_CURRENT_BINARY_DIR}/rpm/RPMS
        COMMAND mkdir -p ${CMAKE_CURRENT_BINARY_DIR}/rpm/SRPMS
        # copy the source of terminalpp to terminalpp directory and add the stamp
        COMMAND rsync -a . ${CMAKE_CURRENT_BINARY_DIR}/rpm/terminalpp --exclude-from=.gitignore --exclude ".git"
        COMMAND cp stamp.h ${CMAKE_CURRENT_BINARY_DIR}/rpm/terminalpp/stamp.h
        # compress the sources and delete the uncompressed ones
        COMMAND tar -czvf ${CMAKE_CURRENT_BINARY_DIR}/rpm/SOURCES/terminalpp.tar.gz -C ${CMAKE_CURRENT_BINARY_DIR}/rpm terminalpp
        COMMAND rm -rf ${CMAKE_CURRENT_BINARY_DIR}/rpm/terminalpp
        # copy the configured spec file
        COMMAND cp ${CMAKE_CURRENT_BINARY_DIR}/configured/rpm/terminalpp.spec ${CMAKE_CURRENT_BINARY_DIR}/rpm/SPECS/terminalpp.spec
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        DEPENDS stamp
    )

    add_custom_target(terminalpp-rpm
        COMMAND rpmbuild --define "_topdir ${CMAKE_CURRENT_BINARY_DIR}/rpm" -ba terminalpp.spec
        COMMAND cp ${CMAKE_CURRENT_BINARY_DIR}/rpm/SRPMS/terminalpp-${PROJECT_VERSION}-1.src.rpm ${CMAKE_BINARY_DIR}/packages/terminalpp.src.rpm
        COMMAND cp ${CMAKE_CURRENT_BINARY_DIR}/rpm/RPMS/x86_64/terminalpp-${PROJECT_VERSION}-1.x86_64.rpm ${CMAKE_BINARY_DIR}/packages/terminalpp.rpm
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/rpm/SPECS
        DEPENDS terminalpp-rpm-prepare
    )

    add_dependencies(packages terminalpp-rpm)

endif()

# macOS
# =====
#
# The package bundle is rather simple, we only need to zip the terminalpp.app already built by terminalpp main project. 
if(PACKAGE_BUNDLE AND RENDERER_QT)
    add_custom_target(terminalpp-bundle
        COMMAND mkdir -p ${CMAKE_BINARY_DIR}/packages
        COMMAND zip -FSr ${CMAKE_BINARY_DIR}/packages/terminalpp.zip terminalpp.app
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/terminalpp
        DEPENDS terminalpp
    )
    add_dependencies(packages terminalpp-bundle)
endif()





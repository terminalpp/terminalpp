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
        COMMAND makepri.exe new /pr . /cf ${CMAKE_CURRENT_SOURCE_DIR}/msix/priconfig.xml /o
        COMMAND ${CMAKE_COMMAND} -E copy
                ${PROJECT_EXECUTABLE}
                VFS/terminalpp.exe
        COMMAND ${CMAKE_COMMAND} -E copy
                ${CMAKE_SOURCE_DIR}/resources/icons/icon_48x48.png
                Assets/icons/icon_48x48.png
        COMMAND ${CMAKE_COMMAND} -E copy
                ${CMAKE_SOURCE_DIR}/resources/icons/icon_128x128.png
                Assets/icons/icon_128x128.png
        COMMAND ${CMAKE_COMMAND} -E copy
                ${CMAKE_SOURCE_DIR}/resources/icons/icon_256x256.png
                Assets/icons/icon_256x256.png
        COMMAND MakeAppx.exe pack /d . /p ${CMAKE_BINARY_DIR}/packages/terminalpp.msix /o
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/msix
        DEPENDS terminalpp
    )
    add_dependencies(packages terminalpp-msix)
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
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/deb/debian/control ${CMAKE_CURRENT_BINARY_DIR}/deb/configured/control @ONLY)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/deb/debian/changelog ${CMAKE_CURRENT_BINARY_DIR}/deb/configured/changelog @ONLY)

    add_custom_target(terminalpp-deb-prepare
        # first clear the output folder
        COMMAND rm -rf ${CMAKE_CURRENT_BINARY_DIR}/deb/terminalpp
        COMMAND mkdir -p ${CMAKE_CURRENT_BINARY_DIR}/deb/terminalpp
        # copy the terminalpp sources
        COMMAND rsync -a . ${CMAKE_CURRENT_BINARY_DIR}/deb/terminalpp --exclude-from=.gitignore --exclude ".git"
        # copy the stamp, the debian folder to the terminalpp root and the configured files
        COMMAND cp stamp.h ${CMAKE_CURRENT_BINARY_DIR}/deb/terminalpp/stamp.h
        COMMAND cp -r ${CMAKE_CURRENT_SOURCE_DIR}/deb/debian ${CMAKE_CURRENT_BINARY_DIR}/deb/terminalpp
        COMMAND cp -f ${CMAKE_CURRENT_BINARY_DIR}/deb/configured/* ${CMAKE_CURRENT_BINARY_DIR}/deb/terminalpp/debian
        # copy the makefile
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




# builds the DEB package via cpack
if(PACKAGE_TERMINALPP AND PACKAGE_DEB AND RENDERER_NATIVE AND ARCH_HAHA)
    set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA "${CMAKE_CURRENT_SOURCE_DIR}/packages/deb/postinst;${CMAKE_CURRENT_SOURCE_DIR}/packages/deb/prerm" PARENT_SCOPE)
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "libfontconfig1, libfreetype6, libpng16-16, libx11-6, libxau6, libxcb1, libxdmcp6, libxft2, libxrender1, x11-utils" PARENT_SCOPE)
    set(CPACK_PACKAGE_DEBIAN_SECTION "utils" PARENT_SCOPE)
endif()

# builds the debian source package
#
# From https://packaging.ubuntu.com/html/packaging-new-software.html
#      https://linuxhint.com/debian-package-creation-howto/

if(PACKAGE_TERMINALPP AND PACKAGE_DEB AND RENDERER_NATIVE AND ARCH_HAHA)
    # first create the source tgz and store it with the packages
    add_custom_target(terminalpp-tgz
        COMMAND rm -rf ${CMAKE_BINARY_DIR}/terminalpp-${PROJECT_VERSION}
        COMMAND mkdir ${CMAKE_BINARY_DIR}/terminalpp-${PROJECT_VERSION}
        COMMAND rsync -a . ${CMAKE_BINARY_DIR}/terminalpp-${PROJECT_VERSION} --exclude-from=.gitignore --exclude ".git"
        COMMAND cp stamp.h ${CMAKE_BINARY_DIR}/terminalpp-${PROJECT_VERSION}/stamp.h
        COMMAND tar -czvf ${CMAKE_BINARY_DIR}/packages/terminalpp-${PROJECT_VERSION}.tar.gz -C ${CMAKE_BINARY_DIR} terminalpp-${PROJECT_VERSION}
        #COMMAND rm -rf ${CMAKE_BINARY_DIR}/terminalpp-${PROJECT_VERSION}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        DEPENDS terminalpp
    )

    # TODO copy & configure the package files - namely control
    # tar xf terminalpp-0.6.0.tar.gz
    # cd terminalpp-0.6.0
    # dh_make -e zduka@outlook.com -f ../terminalpp-0.6.0.tar.gz
    # delete the examples
    #
    # dpkg-buildpackage -b -rfakeroot -us -uc
    # actually -g for source package, but that fails still 
    # then -b would make the debian binary package (i.e. I can get rid of cpack)

endif()

if(PACKAGE_TERMINALPP AND PACKAGE_RPM AND RENDERER_NATIVE AND ARCH_HAHA)
    set(CPACK_RPM_PACKAGE_REQUIRES "fontconfig, libfreetype6, libpng16-16, libX11-6, libXau6, libxcb1, libXdmcp6, libXft2, libXrender1, xmessage" PARENT_SCOPE)
endif()




if(PACKAGE_TERMINALPP AND ARCH_HAHA)
    # cpack settings - generic
    set(CPACK_PACKAGE_DESCRIPTION_SUMMARY ${TPP_SUMMARY})
    set(CPACK_PACKAGE_DESCRIPTION ${TPP_DESCRIPTION})
    # we need to propagate these to the parent scope *and* have them in the local scope because the snap target below might need it
    set(CPACK_PACKAGE_DESCRIPTION_SUMMARY ${CPACK_PACKAGE_DESCRIPTION_SUMMARY}  PARENT_SCOPE)
    set(CPACK_PACKAGE_DESCRIPTION ${CPACK_PACKAGE_DESCRIPTION}  PARENT_SCOPE)
endif()

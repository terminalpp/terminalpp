# Determine tools for package generation

message(STATUS "Looking for package builders...")

# On windows, look for WiX toolset for msi and makeappx for msix
if(ARCH_WINDOWS)
    # WiX toolset 
    if($ENV{WIX})
        set(PACKAGE_MSI $ENV{WIX})
    else()
        file(GLOB PACKAGE_MSI "C:/Program Files*/WiX Toolset v*/bin")
    endif()
    if(PACKAGE_MSI)
        message(STATUS " - msi: wix found in ${PACKAGE_MSI}")
    endif()
    # MakeAppx.exe for msix
    find_program(PACKAGE_MSIX MakeAppx.exe)
    if (PACKAGE_MSIX)
        message(STATUS " - msix: MakeAppx.exe found in ${PACKAGE_MSIX}")
    endif()
endif()

# on Linux, we build snap and deb source package
if(ARCH_LINUX)
    # snapcraft
    find_program(PACKAGE_SNAP snapcraft)
    if(PACKAGE_SNAP)
        message(STATUS " - snap: snapcraft found in ${PACKAGE_SNAP}")
    endif()
    # debian source & binary packages
    # https://help.launchpad.net/Packaging/PPA/BuildingASourcePackage
    find_program(PACKAGE_DEB debuild)
    if(PACKAGE_DEB)
        message(STATUS " - deb: debuild tool found in ${PACKAGE_DEB}")
    endif()
    # rpm source & binary packages
    # http://ftp.rpm.org/max-rpm/
    find_program(PACKAGE_RPM rpmbuild)
    if(PACKAGE_RPM)
        message(STATUS " - rpm: rpmbuild found in ${PACKAGE_RPM}")
    endif()
    
endif()

# on macOS, no extra tools are required to create the bundle
if(ARCH_MACOS)
    set(PACKAGE_BUNDLE true)
    message(STATUS " - bundle")
endif()

# create the packages target
add_custom_target(packages)
set_target_properties(packages PROPERTIES EXCLUDE_FROM_ALL true)

# After the global configuration, include the subprojects for respective packages
add_subdirectory(terminalpp)
add_subdirectory(tpp-bypass)
add_subdirectory(ropen)

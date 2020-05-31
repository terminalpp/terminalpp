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
#add_subdirectory(tpp-bypass)
add_subdirectory(ropen)





if(ARCH_HAHA)

# Package Generation
# ==================
#
# The main project sets up the package generation global settings and searches for all the supported generators. The subprojects, if they are selected by the -DPACKAGE setting use this information to build their own supported packages. 
#
# If no -DPACKAGE argument was provided, terminalpp is selected by default as a package generating project. 
if(NOT DEFINED PACKAGE)
    set(PACKAGE terminalpp)
endif()

# The supplied PACKAGE value is checked and the shorthand value for the selected project is set.
if(PACKAGE STREQUAL terminalpp)
    set(PACKAGE_TERMINALPP true)
elseif(PACKAGE STREQUAL ropen)
    set(PACKAGE_ROPEN true)
elseif(PACKAGE STREQUAL tpp-bypass)
    set(PACKAGE_BYPASS true)
else()
    message(FATAL_ERROR "Invalid value of PACKAGE: ${PACKAGE}")
endif()
message(STATUS "packages: configuring for ${PACKAGE}")

# Generial CPACK variables are set here (such as package name, version, maintainer, license, etc.). The rest of the CPACK values must be set in the respective subprojects within PARENT_SCOPE for the build script to work.  
set(CPACK_PACKAGE_NAME ${PACKAGE})
set(CPACK_PACKAGE_VERSION ${TPP_VERSION})
set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}")
set(CPACK_PACKAGE_VENDOR "Zduka")
set(CPACK_PACKAGE_CONTACT "zduka <zduka@outlook.com>")
# rpm specific
set(CPACK_RPM_PACKAGE_RELEASE 1)
set(CPACK_RPM_PACKAGE_LICENSE "MIT")


endif()
# terminalpp global configuration file

# This is the global configuration file for terminalpp. It is a bit complex as it handles all platforms and configurations as well as the possible subprojects for given platforms (bypass, ropen, etc.)

cmake_minimum_required (VERSION 3.5)

# The Global Project version
# ==========================
#
# For releasing simplicity, all of the packages follow the terminalpp's version numbering. This is the only place where the version should be changed. It will get updated to the subprojects automatically. 
set(TPP_VERSION 0.5.4)

# The pseudoproject for the tools defined in this cmake. 
project(tpp VERSION ${TPP_VERSION})

# architecture macros (ARCH_WINDOWS, ARCH_LINUX, ARCH_BSD, ARCH_MACOS, ARCH_UNIX)
include(${CMAKE_CURRENT_SOURCE_DIR}/helpers/cmake_arch.cmake)

# Global C++ settings
#
# This is a bit complex, C++20 would be best candidate, but it is not supported much and g++ does not even recognized it yet. On msvc and g++ we are therefore happy with c++17.
#
# On clang, the C++20 standard must be used because the __VA_OPT__ usage and the corresponding warning for empty varargs list is disabled.
if(ARCH_MACOS)
    set(CMAKE_CXX_STANDARD 20)
    add_compile_options(-Wno-gnu-zero-variadic-macro-arguments)
else()
    set(CMAKE_CXX_STANDARD 17)
endif()
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# by this all subprojects are addressabe from the source root
include_directories(${CMAKE_SOURCE_DIR})

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

# The supported package generators are now checked. On UNIX systems we check for deb, rpm and snap package generators, on Windows the WIX toolset is searched for. If found, the PACKAGE_xxx variable is set, where xxx is the found packaging tool (DEB, RPM, SNAP, WIX)
if(ARCH_LINUX)
    find_program(PACKAGE_DEB "dpkg")
    if(PACKAGE_DEB)
        message(STATUS "packages: dpkg found in ${PACKAGE_DEB}")
    endif()
    find_program(PACKAGE_RPM "rpmbuild")
    if(PACKAGE_RPM)
        message(STATUS "packages: rpmbuild found in ${PACKAGE_RPM}")
    endif()
    find_program(PACKAGE_SNAP snapcraft)
    if(PACKAGE_SNAP)
        message(STATUS "packages: snapcraft found in ${PACKAGE_SNAP}")
    endif()
elseif(ARCH_WINDOWS)
    if($ENV{WIX})
        set(PACKAGE_WIX $ENV{WIX})
    else()
        file(GLOB PACKAGE_WIX "C:/Program Files*/WiX Toolset v*/bin")
    endif()
    if(PACKAGE_WIX)
        message(STATUS "packages: wix found in ${PACKAGE_WIX}")
    endif()
elseif(ARCH_MACOS)
    set(PACKAGE_APPBUNDLE true)
    message(STATUS "packages: macOS appbundles can be created")
endif()

# Executable stamp creation
# =========================
#
# The stamp is available in the `build_stamp.h` file in the root directory, and is recreated every time any of the source files change. If a target wishes to use the stamp, it must depend on the `stamp` target defined below. 
#
# TODO this is ugly, should be fixed to some nicer dependency when I have time - just list all the sources the stamp creation depends on, i.e. if any of the sources change, the stamp will be recreated.
file(GLOB_RECURSE ALL_SOURCES 
  "helpers/*.h"
  "ropen/*.h"
  "ropen/*.cpp"
  "terminalpp/*.h"
  "terminalpp/*.cpp" 
  "tests/*.h"
  "tests/*.cpp" 
  "tools/*.h"
  "tools/*.cpp"
  "tpp-bypass/*.h"
  "tpp-bypass/*.cpp"
  "tpp-lib/*.h"
  "tpp-lib/*.cpp"
  "ui/*.h"
  "ui/*.cpp"
  "ui-terminal/*.h"
  "ui-terminal/*.cpp"
  "*/CMakeLists.txt"
  "CMakeLists.txt"
)

# Build the stamp generator tool - this is a simple tool that analyzes the github repository as well as other sources and determines the build properties which are then stored in the header file when the generator is run. 
add_executable(
    stamp-generator
	tools/stamp.cpp
)

# A custom command to create the stamp header.
add_custom_command(
  OUTPUT ${CMAKE_SOURCE_DIR}/build_stamp.h
  COMMAND stamp-generator ${CMAKE_SOURCE_DIR} build_stamp.h
  DEPENDS tools/stamp.cpp ${ALL_SOURCES}
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

# A target to depend on - this is needed because the custom command won't work well with parallel builds.
add_custom_target(stamp
    DEPENDS ${ALL_SOURCES} ${CMAKE_SOURCE_DIR}/build_stamp.h
)

# Subprojects
# ===========
#
# All libraries and executables produced by the main buildscript are included now. Some subprojects only make sense for certain platforms, see their respective CMakeLists.txt for details.

add_subdirectory("tpp-lib")
add_subdirectory("ui")
add_subdirectory("ui-terminal")
add_subdirectory("docs")
add_subdirectory("tests")
if(PACKAGE_ROPEN)
    add_subdirectory("ropen")
elseif(PACKAGE_BYPASS)
    add_subdirectory("tpp-bypass")
elseif(PACKAGE_TERMINALPP)
    add_subdirectory("terminalpp")
endif()

# Other tools
# ===========
#
# Creates the appropriate icons for both windows and linux versions. However, since imagemagick is used the target itself is only supported on Linux. 
#
# The generated resources are part of the repository and should only be reran when the original resource files change. 
if(ARCH_LINUX)
    message(STATUS "targets: icons target available for updating terminalpp icons")
    # the xIconCpp is a simple C++ program which takes a RGBA multisize icons and converts it to the format required by the X server, which is then stored as a C++ literal array in specified header file 
    add_executable(
        xIconCpp EXCLUDE_FROM_ALL
        tools/xIconCpp.cpp
    )
    set_target_properties(xIconCpp PROPERTIES EXCLUDE_FROM_ALL TRUE)
    # creates the win32 icon from the logo and the logo with notification, then converts the icons to RGBA a creates the c++ header files containing their contents as literals
    # sizes from the icon exported must be sequential, i.e. all sizes in the icon from the smallest to the largest exported must appear in the as arguments to xIconCpp
    add_custom_target(icons 
        # create icon files for Windows 
        COMMAND convert -background transparent ../images/logo.png -define icon:auto-resize=16,24,32,48,64,72,96,128,256 icon.ico
        COMMAND convert -background transparent ../images/logo-notification.png -define icon:auto-resize=16,24,32,48,64,72,96,128,256 icon-notification.ico
        # create rgba icons for X11
        COMMAND convert icon.ico -depth 8 icon.rgba
        COMMAND xIconCpp tppIcon ${CMAKE_SOURCE_DIR}/terminalpp/x11/resources/tppIcon.cpp icon.rgba 16 24 32 48 64
        COMMAND rm icon.rgba
        COMMAND convert icon-notification.ico -depth 8 icon.rgba
        COMMAND xIconCpp tppIconNotification ${CMAKE_SOURCE_DIR}/terminalpp/x11/resources/tppIconNotification.cpp icon.rgba 16 24 32 48 64
        COMMAND rm icon.rgba
        # create the different sizes of icons used for desktops, qt and so on
        COMMAND convert ../images/logo.png -resize 16x16 icon_16x16.png
        COMMAND convert ../images/logo-notification.png -resize 16x16 icon-notification_16x16.png
        COMMAND convert ../images/logo.png -resize 32x32 icon_32x32.png
        COMMAND convert ../images/logo-notification.png -resize 32x32 icon-notification_32x32.png
        COMMAND convert ../images/logo.png -resize 48x48 icon_48x48.png
        COMMAND convert ../images/logo-notification.png -resize 48x48 icon-notification_48x48.png
        COMMAND convert ../images/logo.png -resize 64x64 icon_64x64.png
        COMMAND convert ../images/logo-notification.png -resize 64x64 icon-notification_64x64.png
        COMMAND convert ../images/logo.png -resize 128x128 icon_128x128.png
        COMMAND convert ../images/logo-notification.png -resize 128x128 icon-notification_128x128.png
        COMMAND convert ../images/logo.png -resize 256x256 icon_256x256.png
        COMMAND convert ../images/logo-notification.png -resize 256x256 icon-notification_256x256.png
        COMMAND convert ../images/logo.png -resize 512x512 icon_512x512.png
        COMMAND convert ../images/logo-notification.png -resize 512x512 icon-notification_512x512.png
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/resources/icons
        DEPENDS xIconCpp
    )
    set_target_properties(icons PROPERTIES EXCLUDE_FROM_ALL TRUE)
elseif(ARCH_MACOS)
    add_custom_target(icons
        COMMAND mkdir bundle.iconset
        COMMAND cp icon_16x16.png bundle.iconset/icon_16x16.png
        COMMAND cp icon_32x32.png bundle.iconset/icon_32x32.png
        COMMAND cp icon_128x128.png bundle.iconset/icon_128x128.png
        COMMAND cp icon_256x256.png bundle.iconset/icon_256x256.png
        COMMAND cp icon_512x512.png bundle.iconset/icon_512x512.png
        COMMAND iconutil -c icns bundle.iconset
        COMMAND rm -rf bundle.iconset
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/resources/icons
    )
    set_target_properties(icons PROPERTIES EXCLUDE_FROM_ALL TRUE)
endif()

# Other Targets
# =============
#
# A simple counter of lines of code in the entire project. Note that this is only useful if all repos have been downloaded first. 
if(ARCH_UNIX)
    add_custom_target(cloc
        COMMAND cloc helpers ropen terminalpp tests tools tpp-bypass tpp-lib ui ui-terminal
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    )
    set_target_properties(cloc PROPERTIES EXCLUDE_FROM_ALL TRUE)
endif()

# CPack and Packages Targets
# ==========================
#
# Detect if the selected package subproject produced any of the supported packages and if so update the target dependencies and cpack generators accordingly (i.e. add respective cpack generators, or if cpack is not used, add the dependency on the target that produces the package).
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/packages)
add_custom_target(packages)
set_target_properties(packages PROPERTIES EXCLUDE_FROM_ALL TRUE)
if(ARCH_LINUX)
    if(PACKAGE_DEB AND DEFINED CPACK_DEBIAN_PACKAGE_DEPENDS)
        message(STATUS "packages: deb package configured for ${PACKAGE}")
        set(CPACK_GENERATOR "${CPACK_GENERATOR}DEB;")
        add_dependencies(packages package-build)
    endif()
    if(PACKAGE_RPM AND DEFINED CPACK_RPM_PACKAGE_REQUIRES)
        message(STATUS "packages: rpm package configured for ${PACKAGE}")
        set(CPACK_GENERATOR "${CPACK_GENERATOR}RPM;")
        add_dependencies(packages package-build)
    endif()
    if(TARGET package-snap)
        message(STATUS "packages: snap package configured for ${PACKAGE}")
        set_target_properties(package-snap PROPERTIES EXCLUDE_FROM_ALL TRUE)
        add_dependencies(packages package-snap)
    endif()
elseif(ARCH_WINDOWS)
    if(TARGET package-msi)
        message(STATUS "packages: msi installed configured for ${PACKAGE}")
        set_target_properties(package-msi PROPERTIES EXCLUDE_FROM_ALL TRUE)
        add_dependencies(packages package-msi)
    endif()
elseif(ARCH_MACOS)
    if(TARGET package-appbundle)
        message(STATUS "packages: appbundle will be created for ${PACKAGE}")
        set_target_properties(package-appbundle PROPERTIES EXCLUDE_FROM_ALL TRUE)
        add_dependencies(packages package-appbundle)
    endif()
endif()

# there is an issue with CMake that does not allow targets depend on special targets such as package. This workaround simply creates a package-build target which explicitly builds the packages by calling cmake. But since it is a user target we can then depend on it
add_custom_target(package-build
    COMMAND "${CMAKE_COMMAND}" --build . --target package
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    COMMENT "Building CPack packages"
    DEPENDS ${PACKAGE}
    VERBATIM
)
set_target_properties(package-build PROPERTIES EXCLUDE_FROM_ALL TRUE)

# Finally, include cpack and set its output directory
set(CPACK_PACKAGE_DIRECTORY "${CMAKE_BINARY_DIR}/packages")
include(CPack)

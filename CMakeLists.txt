# terminalpp global configuration file

# This is the global configuration file for terminalpp. It is a bit complex as it handles all platforms and configurations as well as the possible subprojects for given platforms (bypass, ropen, etc.)

cmake_minimum_required (VERSION 3.5)

if(${CMAKE_VERSION} VERSION_GREATER "3.14")
    cmake_policy(SET CMP0091 NEW)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

# The Global Project and Version
# ==============================
#
# For releasing simplicity, all of the packages follow the terminalpp's version numbering. This is the only place where the version should be changed. It will get updated to the subprojects automatically. 
set(TPP_VERSION 0.7.0)
# The pseudoproject for the tools defined in this cmake. 
project(tpp VERSION ${TPP_VERSION})

set(PROJECT_VENDOR "Zduka")

set(TERMINALPP_SUMMARY "Cross-platform terminal emulator.")
set(TERMINALPP_DESCRIPTION "terminalpp is a minimalist but powerfull terminal emulator which provides the same features and user experience on both Windows, Linux and macOS.")

set(ROPEN_SUMMARY "Remote files opener")
set(ROPEN_DESCRIPTION "Allows terminal++ sessions to open remote files and view them locally by transmitting the file in the existing terminal connection.")


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

# architecture macros (ARCH_WINDOWS, ARCH_LINUX, ARCH_BSD, ARCH_MACOS, ARCH_UNIX)
# and stamp generation macros
include(${CMAKE_CURRENT_SOURCE_DIR}/helpers/helpers.cmake)


# Renderer
# ========
#
# NATIVE, QT, or NONE. 

if(NOT DEFINED RENDERER)
    if(ARCH_MACOS)
        set(RENDERER QT)
    else()
        set(RENDERER NATIVE)
    endif()
endif()

# check correct renderer value and determine the shortcut variables
if(RENDERER STREQUAL NATIVE)
    set(RENDERER_NATIVE true)
    add_definitions(-DRENDERER_NATIVE)
elseif(RENDERER STREQUAL QT)
    set(RENDERER_QT true)
    add_definitions(-DRENDERER_QT)
elseif(RENDERER STREQUAL NONE)
    set(RENDERER_NONE true)
else()
    message(FATAL_ERROR "Unknown renderer ${RENDERER}")
endif()
message(STATUS "Renderer: ${RENDERER}")

if(ARCH_WINDOWS)
    cmake_minimum_required (VERSION 3.15)
    message(STATUS "MSVC Runtime will be linked statically")    
endif()

# by this all subprojects are addressabe from the source root
include_directories(${CMAKE_SOURCE_DIR})

# Subprojects
# ===========
#
# All libraries and executables produced by the main buildscript are included now. Some subprojects only make sense for certain platforms, see their respective CMakeLists.txt for details.
if(PACKAGE_BYPASS OR PACKAGE_TERMINALPP)
    add_subdirectory("tpp-bypass")
endif()
add_subdirectory("tpp-lib")
add_subdirectory("ropen")
add_subdirectory("ui")
add_subdirectory("ui-terminal")
add_subdirectory("docs")
add_subdirectory("tests")
add_subdirectory("terminalpp")

add_subdirectory("packages")

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
        # 1080x1080 box art for windows store
        COMMAND convert ../images/logo.png -resize 1080x1080 icon_1080x1080.png
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
#file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/packages)
#add_custom_target(packages)
#set_target_properties(packages PROPERTIES EXCLUDE_FROM_ALL TRUE)
#if(ARCH_LINUX)
#    if(PACKAGE_DEB AND DEFINED CPACK_DEBIAN_PACKAGE_DEPENDS)
#        message(STATUS "packages: deb package configured for ${PACKAGE}")
#        set(CPACK_GENERATOR "${CPACK_GENERATOR}DEB;")
#        add_dependencies(packages package-build)
#    endif()
#    if(PACKAGE_RPM AND DEFINED CPACK_RPM_PACKAGE_REQUIRES)
#        message(STATUS "packages: rpm package configured for ${PACKAGE}")
#        set(CPACK_GENERATOR "${CPACK_GENERATOR}RPM;")
#        add_dependencies(packages package-build)
#    endif()
#    if(TARGET package-snap)
#        message(STATUS "packages: snap package configured for ${PACKAGE}")
#        set_target_properties(package-snap PROPERTIES EXCLUDE_FROM_ALL TRUE)
#        add_dependencies(packages package-snap)
#    endif()
#elseif(ARCH_WINDOWS)
#    if(TARGET package-msi)
#        message(STATUS "packages: msi installer configured for ${PACKAGE}")
#        set_target_properties(package-msi PROPERTIES EXCLUDE_FROM_ALL TRUE)
#        add_dependencies(packages package-msi)
#    endif()
#    if(TARGET package-msix)
#        message(STATUS "packages: msix installer configured for ${PACKAGE}")
#        set_target_properties(package-msix PROPERTIES EXCLUDE_FROM_ALL TRUE)
#        add_dependencies(packages package-msix)
#    endif()
#elseif(ARCH_MACOS)
#    if(TARGET package-appbundle)
#        message(STATUS "packages: appbundle will be created for ${PACKAGE}")
#        set_target_properties(package-appbundle PROPERTIES EXCLUDE_FROM_ALL TRUE)
#        add_dependencies(packages package-appbundle)
#    endif()
#endif()

# there is an issue with CMake that does not allow targets depend on special targets such as package. This workaround simply creates a package-build target which explicitly builds the packages by calling cmake. But since it is a user target we can then depend on it
#add_custom_target(package-build
#    COMMAND "${CMAKE_COMMAND}" --build . --target package
#    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
#    COMMENT "Building CPack packages"
#    DEPENDS ${PACKAGE}
#    VERBATIM
#)
#set_target_properties(package-build PROPERTIES EXCLUDE_FROM_ALL TRUE)

# Finally, include cpack and set its output directory
#set(CPACK_PACKAGE_DIRECTORY "${CMAKE_BINARY_DIR}/packages")
#include(CPack)

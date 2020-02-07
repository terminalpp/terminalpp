# terminalpp global configuration file

# This is the global configuration file for terminalpp. It is a bit complex as it handles all platforms and configurations as well as the possible subprojects for given platforms (bypass, ropen, etc.)

cmake_minimum_required (VERSION 3.5)

# terminalpp version - this version is stamped on all the executables produced and should be the only place when version should be changed manually.
#
# Version format is MAJOR.MINOR.PATCH. 
project(terminalpp VERSION 0.5.2)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(${CMAKE_CURRENT_SOURCE_DIR}/helpers/cmake_arch.cmake)

include_directories(".")

if(NOT DEFINED PACKAGE_INSTALL)
    set(PACKAGE_INSTALL terminalpp)
endif()

# Determines the renderer to be used
#
# By default, the native renderer is specified, but QT can be specified instead. On macOS where native renderer is not available, QT is the default renderer. 
if(NOT DEFINED RENDERER)
    if(DEFINED APPLE)
        set(RENDERER QT)
    else()
        set(RENDERER NATIVE)
    endif()
endif()
add_definitions(-DRENDERER_${RENDERER})
message(STATUS "renderer: ${RENDERER}")

# A futile attempt at getting the coverage outputs being produced
#if(UNIX)
#    if(NOT "${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
#        message(FATAL_ERROR "Coverage requires GCC.")
#    endif()
#
#    # enable profiling
#    set(CMAKE_CXX_FLAGS "--coverage -g -O0 -fprofile-arcs -ftest-coverage")
#    set(CMAKE_CXX_OUTPUT_EXTENSION_REPLACE 1)
#
#    # from https://github.com/RWTH-HPC/CMake-codecov/blob/master/cmake/FindGcov.cmake
#    get_filename_component(COMPILER_PATH "${CMAKE_CXX_COMPILER}" PATH)
#    string(REGEX MATCH "^[0-9]+" GCC_VERSION "${CMAKE_CXX_COMPILER_VERSION}")
#    find_program(GCOV_BIN NAMES gcov-${GCC_VERSION} gcov HINTS ${COMPILER_PATH})
#    message(STATUS "Gcov found in ${GCOV_BIN}")
#
#
#endif()

# Include sub-projects.
add_subdirectory("tpp-lib")
add_subdirectory("ui")
add_subdirectory("ui-terminal")
add_subdirectory("docs")
add_subdirectory("tests")
# optional subprojects - only include them if we actually have them
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/ropen/CMakeLists.txt")
    add_subdirectory("ropen")
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tpp-bypass/CMakeLists.txt")
    add_subdirectory("tpp-bypass")
endif()

# Build checks
#
# Only on Linux, check for the required libraries. 
if(UNIX)
    find_package(Threads REQUIRED)
    if(NOT APPLE)
        find_package(X11 REQUIRED)
        find_package(Freetype REQUIRED)
        include_directories(${X11_INCLUDE_DIR})
        include_directories(${FREETYPE_INCLUDE_DIRS})
        if (NOT X11_Xft_FOUND)
            message(FATAL_ERROR "xft not found - please install libxft-dev")
        endif()
    endif()
endif()

# Check that QT is installed if QT renderer selected
# 
# On Windows, the Qt path is specified manually. Is there a better way to do it, and should I bother?
if(RENDERER STREQUAL QT)
    if(WIN32)
        set(CMAKE_PREFIX_PATH "C:\\Qt\\5.14.1\\msvc2017_64")
    endif()
    set(CMAKE_AUTOMOC ON)
    set(CMAKE_INCLUDE_CURRENT_DIR ON)
    set_property(SOURCE build-stamp.h PROPERTY SKIP_AUTOMOC ON)
    find_package(Qt5Widgets REQUIRED)
    if(WIN32)
        # finds the windeployqt executable so that the binaries can be added
        find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS "${_qt_bin_dir}")
    endif()
endif()

# tpp executable
#
# On Windows, this is simply a Win32 GUI application created from the given sources and resources file. On Linux the situation is more complex as the build scripts tries to localize all required libraries and then build the app. 
file(GLOB_RECURSE SRC "terminalpp/*.cpp" "terminalpp/*.h")
if(WIN32)
    add_executable(terminalpp WIN32 ${SRC} "terminalpp/directwrite/tpp.rc" "build-stamp.h")
    # add QT dlls to the executable
    if(RENDERER STREQUAL QT)
        add_custom_command(TARGET terminalpp POST_BUILD
            COMMAND ${WINDEPLOYQT_EXECUTABLE} "$<TARGET_FILE:terminalpp>"
        )
    endif()
elseif(UNIX)
    add_executable(terminalpp ${SRC} "build-stamp.h")
    target_link_libraries(terminalpp ${CMAKE_THREAD_LIBS_INIT})
    if(RENDERER STREQUAL NATIVE)
        target_link_libraries(terminalpp ${X11_LIBRARIES} ${X11_Xft_LIB})  
        target_link_libraries(terminalpp ${FREETYPE_LIBRARIES})
        target_link_libraries(terminalpp fontconfig)
    endif()
    if(NOT APPLE)
        target_link_libraries(terminalpp stdc++fs)    
    endif()
    # TODO newer cmake version supports FindFontconfig which should be used instead (3.14 upwards)
endif()
# finally, add the libraries
target_link_libraries(terminalpp libui libuiterminal)

# link with QT libraries if selected
if(RENDERER STREQUAL QT)
    target_link_libraries(terminalpp Qt5::Widgets)
endif()

# Tools
#
# A simple counter of lines of code in the entire project. Note that this is only useful if all repos have been downloaded first. 
if(UNIX)
    add_custom_target(cloc
        COMMAND cloc helpers ropen terminalpp tests tools tpp-bypass tpp-lib ui ui-terminal
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    )
    set_target_properties(cloc PROPERTIES EXCLUDE_FROM_ALL TRUE)
endif()

# Executable stamp creation
#
# First an executable stamp is created which is then invoked via the tpp-create-stamp target to produce the stamp.h file containing the stamp data. 
add_executable(
    stamp
	tools/stamp.cpp
)

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
)

add_custom_command(
  OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/build-stamp.h
  COMMAND stamp ${CMAKE_CURRENT_SOURCE_DIR} build-stamp.h
  DEPENDS tools/stamp.cpp stamp ${ALL_SOURCES}
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

# Resources preparation - icons 
#
# Creates the appropriate icons for both windows and linux versions. However, since imagemagick is used the target itself is only supported on Linux. 
#
# The generated resources are part of the repository and should only be reran when the original resource files change. 
if(UNIX AND NOT APPLE)
    # the xIconCpp is a simple C++ program which takes a RGBA multisize icons and converts it to the format required by the X server, which is then stored as a C++ literal array in specified header file 
    add_executable(
        xIconCpp EXCLUDE_FROM_ALL
        tools/xIconCpp.cpp
    )
    set_target_properties(xIconCpp PROPERTIES EXCLUDE_FROM_ALL TRUE)
    # creates the win32 icon from the logo and the logo with notification, then converts the icons to RGBA a creates the c++ header files containing their contents as literals
    add_custom_target(icons 
        COMMAND convert -background transparent ${CMAKE_CURRENT_SOURCE_DIR}/resources/images/logo.png -define icon:auto-resize=16,24,32,48,64,72,96,128,256 tpp.ico
        COMMAND convert -background transparent ${CMAKE_CURRENT_SOURCE_DIR}/resources/images/logo-notification.png -define icon:auto-resize=16,24,32,48,64,72,96,128,256 tpp-notification.ico
        COMMAND convert tpp.ico -depth 8 tpp.rgba
        COMMAND xIconCpp tppIcon ${CMAKE_CURRENT_SOURCE_DIR}/terminalpp/x11/tppIcon.cpp tpp.rgba 16 24 32 64
        COMMAND rm tpp.rgba
        COMMAND convert tpp-notification.ico -depth 8 tpp.rgba
        COMMAND xIconCpp tppIconNotification ${CMAKE_SOURCE_DIR}/terminalpp/x11/tppIconNotification.cpp tpp.rgba 16 24 32 64
        COMMAND rm tpp.rgba
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/resources/images
        DEPENDS xIconCpp
    )
    set_target_properties(icons PROPERTIES EXCLUDE_FROM_ALL TRUE)
endif()

# Packages
#
# The packages target generates packages such as msi or snap installers, deb or rpm packages, etc. Since the generation of packages is not necessary for building from source, the packages target is only populated if the required tools are found. 
#
# The packages target is first created empty and then the buildres register as its dependencies if they are available. 
#
# All assets are stored in ${CMAKE_CURRENT_BINARY_DIR}/packages.
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/packages)
add_custom_target(packages)
set_target_properties(packages PROPERTIES EXCLUDE_FROM_ALL TRUE)

# there is an issue with CMake that does not allow targets depend on special targets such as package. This workaround simply creates a package-build target which explicitly builds the packages by calling cmake. But since it is a user target we can then depend on it
add_custom_target(package-build
    COMMAND "${CMAKE_COMMAND}" --build . --target package
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    COMMENT "Building CPack packages"
    DEPENDS terminalpp
    VERBATIM
)
set_target_properties(package-build PROPERTIES EXCLUDE_FROM_ALL TRUE)

# Linux DEB and RPM packages
#
# CPack is used to produce these. If the required tools are found, the respective CPack generators are added.
if(UNIX) 
    find_program(DPKG "dpkg")
    if(DPKG)
        message(STATUS "dpkg found in ${DPKG}, DEB package will be added to packages")
        set(CPACK_GENERATOR "${CPACK_GENERATOR}DEB;")
        add_dependencies(packages package-build)
    endif()
    find_program(RPMBUILD "rpmbuild")
    if (RPMBUILD)
        message(STATUS "rpmbuild found in ${RPMBUILD}, RPM package will be added to packages")
        set(CPACK_GENERATOR "${CPACK_GENERATOR}RPM;")
        add_dependencies(packages package-build)
    endif()
endif()


# Include cpack which adds the package target
# cpack settings - generic
set(CPACK_PACKAGE_NAME ${PACKAGE_INSTALL})
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}")
set(CPACK_PACKAGE_VENDOR "Zduka")
set(CPACK_PACKAGE_CONTACT "zduka <zduka@outlook.com>")
# debian specific
set(CPACK_PACKAGE_DEBIAN_SECTION "utils")
# rpm specific
set(CPACK_RPM_PACKAGE_RELEASE 1)
set(CPACK_RPM_PACKAGE_LICENSE "MIT")

if(PACKAGE_INSTALL STREQUAL terminalpp)
    message(STATUS "terminalpp project package will be built")
    if(UNIX)
        install(TARGETS terminalpp DESTINATION bin COMPONENT terminalpp)
    endif()

    # cpack settings - generic
    set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Cross-platform terminal emulator.")
    set(CPACK_PACKAGE_DESCRIPTION "terminalpp is a minimalist but powerfull terminal emulator which provides the same features and user experience on both Windows and Linux.")
    # debian specific
    set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA "${CMAKE_CURRENT_SOURCE_DIR}/packages/deb/postinst;${CMAKE_CURRENT_SOURCE_DIR}/packages/deb/prerm")
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "libfontconfig1, libfreetype6, libpng16-16, libx11-6, libxau6, libxcb1, libxdmcp6, libxft2, libxrender1, x11-utils")
    # rpm specific
    set(CPACK_RPM_PACKAGE_REQUIRES "fontconfig, libfreetype6, libpng16-16, libX11-6, libXau6, libxcb1, libXdmcp6, libXft2, libXrender1, xmessage")


    # WiX powered MSI installer creation (Windows)
    #
    # Creates the MSI installer (classic Windows desktop installer). 
    if(WIN32)
        # Search for the wix toolkit 
        if (DEFINED $ENV{WIX})
            set(WIX_ROOT $ENV{WIX})
        else()   
            file(GLOB WIX_ROOT "C:/Program Files*/WiX Toolset v*/bin")
        endif()
        if (WIX_ROOT) 
            message(STATUS "WIX toolset found in ${WIX_ROOT}, msi package creation will be added to packages")
            # create the build directory and configure the WiX configuration in it
            configure_file(${CMAKE_CURRENT_SOURCE_DIR}/packages/msi/tpp.wxs ${CMAKE_CURRENT_BINARY_DIR}/packages/msi/tpp.wxs @ONLY)
            add_custom_target(package-msi
                COMMAND ${WIX_ROOT}/candle.exe
                    -nologo 
                    -arch "x64" 
                    -ext WixUIExtension -ext WixUtilExtension 
                    -out "tpp.wixobj" 
                    "tpp.wxs"
                COMMAND ${WIX_ROOT}/light.exe 
                    -nologo
                    -ext WixUIExtension
                    -ext WixUtilExtension 
                    -out "../${PROJECT_NAME}.msi" 
                    -sice:ICE61 -sice:ICE91 
                    "tpp.wixobj"
                WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/packages/msi
                DEPENDS terminalpp
            )        
            set_target_properties(package-msi PROPERTIES EXCLUDE_FROM_ALL TRUE)
            add_dependencies(packages package-msi)
        endif()
    endif() 

    # Builds the snap package for linux
    #
    # The classic confinement is used as terminal with the strict confinement is most likely not what we want.
    if(UNIX)
        find_program(SNAP snapcraft)
        if(SNAP)
            message(STATUS "snapcraft found in ${SNAP}, snap package will be added to packages")
            message(STATUS "snapcraft extra args: ${SNAP_EXTRA_ARGS}")
            # create the build directory and configure the WiX configuration in it
            configure_file(${CMAKE_CURRENT_SOURCE_DIR}/packages/snap/tpp.yaml ${CMAKE_CURRENT_BINARY_DIR}/snap/snapcraft.yaml @ONLY)
            add_custom_target(package-snap-copy-tpp
                COMMAND rm -rf ${CMAKE_CURRENT_BINARY_DIR}/snap/source
                COMMAND mkdir ${CMAKE_CURRENT_BINARY_DIR}/snap/source
                COMMAND rsync -a . ${CMAKE_CURRENT_BINARY_DIR}/snap/source  --exclude-from=.gitignore
                WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            )
            set_target_properties(package-snap-copy-tpp PROPERTIES EXCLUDE_FROM_ALL TRUE)
            add_custom_target(package-snap
                COMMAND snapcraft clean ${SNAP_EXTRA_ARGS}
                COMMAND snapcraft ${SNAP_EXTRA_ARGS}
                COMMAND cp ${PROJECT_NAME}_${PROJECT_VERSION}_amd64.snap ${CMAKE_CURRENT_BINARY_DIR}/packages/${PROJECT_NAME}.snap
                WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/snap
                DEPENDS package-snap-copy-tpp
            )
            set_target_properties(package-snap PROPERTIES EXCLUDE_FROM_ALL TRUE)
            add_dependencies(packages package-snap)
        endif()
    endif()

elseif(PACKAGE_INSTALL STREQUAL tpp-ropen)
    message(STATUS "ropen project package will be built")
    # cpack settings - generic
    set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Remote file opener.")
    set(CPACK_PACKAGE_DESCRIPTION "Allows terminal++ sessions to open remote files and view them locally by transmitting the file in the existing terminal connection.")
elseif(PACKAGE_INSTALL STREQUAL tpp-bypass)
    message(STATUS "bypass project package will be built")
    # cpack settings - generic
    set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "ConPTY Bypass.")
    set(CPACK_PACKAGE_DESCRIPTION "Bypasses terminal IO to standard input and output to bypass the ConPTY on Windows when WSL is used. ")
endif()

if(PACKAGE_INSTALL)
    set(CPACK_PACKAGE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/packages")
    include(CPack)
endif()


# CMakeList.txt : Top-level CMake project file, do global configuration
# and include sub-projects here.
#
cmake_minimum_required (VERSION 3.5)

project(tpp VERSION 0.2)

include(${CMAKE_CURRENT_SOURCE_DIR}/helpers/cmake_arch.cmake)

include_directories(".")

# Include sub-projects.
add_subdirectory("vterm")
add_subdirectory("ui")
add_subdirectory("docs")


#include_directories("{$CMAKE_CURRENT_SOURCE_DIR}/tpp")

# Build checks
#
# Only on Linux, check for the required libraries. 
if (UNIX)
    find_package(X11 REQUIRED)
    find_package(Threads REQUIRED)
    find_package(Freetype REQUIRED)
    message(${LUTIL})
    include_directories(${X11_INCLUDE_DIR})
    include_directories(${FREETYPE_INCLUDE_DIRS})
    if (NOT X11_Xft_FOUND)
        message(FATAL_ERROR "xft not found - please install libxft-dev")
    endif()
endif()

# tpp executable
#
# On Windows, this is simply a Win32 GUI application created from the given sources and resources file. On Linux the situation is more complex as the build scripts tries to localize all required libraries and then build the app. 
file(GLOB_RECURSE SRC "tpp/*.cpp" "tpp/*.h")
if(WIN32)
    add_executable(tpp WIN32 ${SRC} "tpp/directwrite/tpp.rc")
elseif(UNIX)
    add_executable(tpp ${SRC})
    target_link_libraries(tpp ${X11_LIBRARIES} ${X11_Xft_LIB})  
    target_link_libraries(tpp ${CMAKE_THREAD_LIBS_INIT})
    target_link_libraries(tpp ${FREETYPE_LIBRARIES})
    # TODO newer cmake version supports FindFontconfig which should be used instead (3.14 upwards)
    target_link_libraries(tpp fontconfig)
endif()
# finally, add the libraries
target_link_libraries(tpp libvterm libui)
# add the dependency on stamp creation tool (see below)
add_dependencies(tpp tpp-create-stamp)

# Executable stamp creation
#
# First an executable stamp is created which is then invoked via the tpp-create-stamp target to produce the stamp.h file containing the stamp data. 
add_executable(
    stamp
	tools/stamp.cpp
)
add_custom_target(tpp-create-stamp
    COMMAND stamp ${CMAKE_CURRENT_SOURCE_DIR} stamp.h
)
add_dependencies(tpp-create-stamp stamp)

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
        COMMAND xIconCpp tppIcon ${CMAKE_CURRENT_SOURCE_DIR}/tpp/x11/tppIcon.cpp tpp.rgba 16 24 32 64
        COMMAND rm tpp.rgba
        COMMAND convert tpp-notification.ico -depth 8 tpp.rgba
        COMMAND xIconCpp tppIconNotification ${CMAKE_SOURCE_DIR}/tpp/x11/tppIconNotification.cpp tpp.rgba 16 24 32 64
        COMMAND rm tpp.rgba
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/resources/images
    )
    add_dependencies(icons xIconCpp)
    set_target_properties(icons PROPERTIES EXCLUDE_FROM_ALL TRUE)
endif()

# Assets
#
# The assets target generates assets such as msi or snap installers, etc. Since the generation of assets is not necessary for building from source, the assets target is only created if the required tools are found. 
#
# The assets target is first created empty and then the asset buildres register as its dependencies if they are available. 
#
# All assets are stored in ${CMAKE_BUILD_DIR}/assets
add_custom_target(assets)
set_target_properties(assets PROPERTIES EXCLUDE_FROM_ALL TRUE)

# Linux make install settings
#
# This is not really an asset, but this is where it belongs the best
if(UNIX)
    install(TARGETS tpp DESTINATION bin)
endif()

# WiX powered MSI installer creation (Windows)
#
# Creates the MSI installer (classic Windows desktop installer). 
if(WIN32)
    # Search for the wix toolkit    
    file(GLOB WIX_ROOT "C:/Program Files*/WiX Toolset v*/bin")
    if (WIX_ROOT) 
        message(STATUS "WIX toolset found in ${WIX_ROOT}, msi package creation will be added to assets")
        # create the build directory and configure the WiX configuration in it
        configure_file(${CMAKE_CURRENT_SOURCE_DIR}/assets/msi/tpp.wxs ${CMAKE_CURRENT_BINARY_DIR}/assets/msi/tpp.wxs @ONLY)
        add_custom_target(assets-msi
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
                -out "tpp.msi" 
                -sice:ICE61 -sice:ICE91 
                "tpp.wixobj"
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/assets/msi
        )        
        set_target_properties(assets-msi PROPERTIES EXCLUDE_FROM_ALL TRUE)
        add_dependencies(assets assets-msi)
    endif()
endif() 

# Builds the snap package for linux
#
# Under development. 
if(UNIX)
    find_program(SNAP snapcraft)
    if(SNAP)
        message(STATUS "snapcraft found in ${SNAP}, snap package will be added to assets")
        # create the build directory and configure the WiX configuration in it
        configure_file(${CMAKE_CURRENT_SOURCE_DIR}/assets/snap/tpp.yaml ${CMAKE_CURRENT_BINARY_DIR}/assets/snap/snapcraft.yaml @ONLY)
        add_custom_target(assets-snap
            COMMAND snapcraft
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/assets/snap
            )
        set_target_properties(assets-snap PROPERTIES EXCLUDE_FROM_ALL TRUE)
        add_dependencies(assets assets-snap)
    endif()
endif()

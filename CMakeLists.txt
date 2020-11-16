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
set(TPP_VERSION 0.8.0.1)
# The pseudoproject for the tools defined in this cmake. 
project(tpp VERSION ${TPP_VERSION})

set(PROJECT_VENDOR "Zduka")

set(TERMINALPP_SUMMARY "Cross-platform terminal emulator.")
set(TERMINALPP_DESCRIPTION "terminalpp is a minimalist but powerfull terminal emulator which provides the same features and user experience on both Windows, Linux and macOS.")

set(ROPEN_SUMMARY "Remote files opener")
set(ROPEN_DESCRIPTION "Allows terminal++ sessions to open remote files and view them locally by transmitting the file in the existing terminal connection.")

set(BYPASS_SUMMARY "ConPTY Bypass.")
set(BYPASS_DESCRIPTION "Bypasses terminal IO to standard input and output to bypass the ConPTY on Windows when WSL is used. ")

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
  "tpp-server/*.h"
  "tpp-server/*.cpp"
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
add_subdirectory("tpp-bypass")
add_subdirectory("tpp-server")
add_subdirectory("tpp-lib")
add_subdirectory("ropen")
add_subdirectory("ui")
add_subdirectory("ui-terminal")
add_subdirectory("docs")
add_subdirectory("tests")
add_subdirectory("terminalpp")
add_subdirectory("tools")
add_subdirectory("packages")

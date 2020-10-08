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
set(TPP_VERSION 0.7.99)
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

# platform detection 
if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    message(STATUS "Windows platform detected")
    add_definitions(-DARCH_WINDOWS)
    add_definitions(-DARCH=\"Windows\")
    set(ARCH_WINDOWS true)
# https://stackoverflow.com/questions/21995777/cygwins-cmake-does-not-match-for-cmake-system-name
elseif("${CMAKE_SYSTEM_NAME}" STREQUAL "MSYS")
    message(STATUS "MSYS platform detected")
    add_definitions(-DARCH_UNIX)
    add_definitions(-DARCH_LINUX)
    add_definitions(-DARCH=\"Linux\")
    set(ARCH_UNIX true)
    set(ARCH_LINUX true)
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    message(STATUS "Linux platform detected")
    add_definitions(-DARCH_UNIX)
    add_definitions(-DARCH_LINUX)
    add_definitions(-DARCH=\"Linux\")
    set(ARCH_UNIX true)
    set(ARCH_LINUX true)
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    message(STATUS "macOS platform detected")
    add_definitions(-DARCH_UNIX)
    add_definitions(-DARCH_MACOS)
    add_definitions(-DARCH=\"macOS\")
    set(ARCH_UNIX true)
    set(ARCH_MACOS true)
elseif(${CMAKE_SYSTEM_NAME} MATCHES "FreeBSD")
    message(STATUS "BSD platform detected")
    add_definitions(-DARCH_UNIX)
    add_definitions(-DARCH_BSD)
    add_definitions(-DARCH=\"FreeBSD\")
    set(ARCH_UNIX true)
    set(ARCH_BSD true)
else()
    message(FATAL_ERROR "Unsupported platform detected: '${CMAKE_SYSTEM_NAME}' ${XXX}")
endif()

if (${CMAKE_SIZEOF_VOID_P} EQUAL "8")
  add_definitions(-DARCH_SIZE=64)
elseif (${CMAKE_SIZEOF_VOID_P} EQUAL "4")
  add_definitions(-DARCH_SIZE=32)
else()
endif()

add_definitions(-DARCH_COMPILER=\"${CMAKE_CXX_COMPILER_ID}\")
add_definitions(-DARCH_COMPILER_VERSION=\"${CMAKE_CXX_COMPILER_VERSION}\")

if(MSVC)
  add_compile_options(/W4)
  add_compile_definitions(NOMINMAX)
  add_compile_definitions(_UNICODE)
  add_compile_definitions(UNICODE)
  add_definitions(-DARCH_WINDOWS)
  add_definitions(-DARCH=\"Windows\")
else()
  add_compile_options(-std=c++17 -Wall -Wextra -pedantic)
endif()

# add project name and version as set by cmake to macros as well
add_definitions(-DPROJECT_NAME=\"${PROJECT_NAME}\")
add_definitions(-DPROJECT_VERSION=\"${PROJECT_VERSION}\")

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

# Executable stamp creation
# =========================
#
# The stamp is available in the `build_stamp.h` file in the root directory, and is recreated every time any of the source files change. If a target wishes to use the stamp, it must depend on the `stamp` target defined below. 
#
# TODO this is ugly, should be fixed to some nicer dependency when I have time - just list all the sources the stamp creation depends on, i.e. if any of the sources change, the stamp will be recreated.



# Build the stamp generator tool - this is a simple tool that analyzes the github repository as well as other sources and determines the build properties which are then stored in the header file when the generator is run. 
add_executable(
    stamp-generator
	helpers/stamp_generator.cpp
)
if(ARCH_LINUX)
    target_link_libraries(stamp-generator stdc++fs)    
endif()

# A custom command to create the stamp header.
add_custom_command(
  OUTPUT ${CMAKE_SOURCE_DIR}/stamp.h
  COMMAND stamp-generator ${CMAKE_SOURCE_DIR}/stamp.h ${STAMP_ARGS}
  DEPENDS ${ALL_SOURCES}
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

# A target to depend on - this is needed because the custom command won't work well with parallel builds.
add_custom_target(stamp
    DEPENDS ${ALL_SOURCES} ${CMAKE_SOURCE_DIR}/stamp.h
)

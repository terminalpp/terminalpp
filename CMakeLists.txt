# CMakeList.txt : Top-level CMake project file, do global configuration
# and include sub-projects here.
#
cmake_minimum_required (VERSION 3.5)


# platform detection 
if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
  add_definitions(-DARCH_WINDOWS)
  add_definitions(-DARCH=\"Windows\")
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
  add_definitions(-DARCH_UNIX)
  add_definitions(-DARCH_LINUX)
  add_definitions(-DARCH=\"Linux\")
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  add_definitions(-DARCH_UNIX)
  add_definitions(-DARCH_MACOS)
  add_definitions(-DARCH=\"macOS\")
elseif(${CMAKE_SYSTEM_NAME} MATCHES "FreeBSD")
  add_definitions(-DARCH_UNIX)
  add_definitions(-DARCH_BSD)
  add_definitions(-DARCH=\"FreeBSD\")
else()
# add error that unsupported platform is detected
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

include_directories(".")

# Include sub-projects.

add_subdirectory("tools")

add_subdirectory("vterm")

add_subdirectory("ui")

add_subdirectory("tpp")

add_subdirectory("bypass")


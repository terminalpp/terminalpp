# CMakeList.txt : Top-level CMake project file, do global configuration
# and include sub-projects here.
#
cmake_minimum_required (VERSION 3.5)

if(MSVC)
  add_compile_options(/W4)
  add_compile_definitions(NOMINMAX)
  add_compile_definitions(_UNICODE)
  add_compile_definitions(UNICODE)
else()
  add_compile_options(-std=c++11 -Wall -Wextra -pedantic)
endif()

include_directories(".")

# Include sub-projects.

add_subdirectory("tools")

add_subdirectory("vterm")

add_subdirectory("tpp")

add_subdirectory("bypass")


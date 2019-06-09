# CMakeList.txt : Top-level CMake project file, do global configuration
# and include sub-projects here.
#
cmake_minimum_required (VERSION 3.5)

if(MSVC)
  add_compile_options(/W4)
else()
  add_compile_options(-Wall -Wextra -pedantic)
endif()

include_directories(".")

# Include sub-projects.

add_subdirectory("vterm")

add_subdirectory("tpp")

if(UNIX)
    add_subdirectory("tserv")
endif()


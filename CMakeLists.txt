# CMakeList.txt : Top-level CMake project file, do global configuration
# and include sub-projects here.
#
cmake_minimum_required (VERSION 3.5)

include_directories(".")

# Include sub-projects.

add_subdirectory("vterm")

add_subdirectory("tpp")


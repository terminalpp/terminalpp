# CMakeList.txt : CMake project for t++, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

project(tpp)

include_directories(".")

file(GLOB_RECURSE SRC "*.cpp" "*.h")

if (WIN32)
    add_compile_definitions(TPP_RENDERED=gdi)
    add_executable(tpp WIN32 ${SRC})
else (WIN32)
    add_executable(tpp ${SRC})
endif (WIN32)

target_link_libraries(tpp libvterm)

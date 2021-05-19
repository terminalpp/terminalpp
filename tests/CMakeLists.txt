# Tests
#
# A simple executable target for the tests is created from all test sources

cmake_minimum_required (VERSION 3.5)

file(GLOB_RECURSE TESTS_HELPERS "../helpers/tests/*.h" "../helpers/tests/*.cpp")
file(GLOB_RECURSE TESTS_UI "../ui/tests/*.h" "../ui/tests/*.cpp")
file(GLOB_RECURSE TESTS_UI4 "../ui4/tests/*.h" "../ui4/tests/*.cpp")
file(GLOB_RECURSE TESTS_UI_TERM "../ui-terminal/tests/*.h" "../ui-terminal/tests/*.cpp")

#if(UNIX)
#    SET(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -g -O0 --coverage")
#    SET(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} --coverage")
#endif()

add_executable(tests "main-tests.cpp" ${TESTS_HELPERS} ${TESTS_UI} ${TESTS_UI4} ${TESTS_UI_TERM})
target_link_libraries(tests libui libui4)

#if(UNIX)
#    set(GCOV "gcov-8")
#    add_custom_target(coverage
        #COMMAND lcov --gcov-tool ${GCOV} -directory . --zerocounters
        #COMMAND lcov --gcov-tool ${GCOV} -c -i -d . -o coverage.base
        #COMMAND tests
        #COMMAND lcov --gcov-tool ${GCOV} --directory . --capture --output-file coverage.info
        #COMMAND lcov --gcov-tool ${GCOV} -a coverage.base -a coverage.info --output-file coverage.total
        ##COMMAND lcov --gcov-tool ${GCOV} --remove coverage.total ${COVERAGE_LCOV_EXCLUDES} --output-file ${PROJECT_BINARY_DIR}/${Coverage_NAME}.info.cleaned
        #COMMAND genhtml  -o coverage ${PROJECT_BINARY_DIR}/coverage.info.cleaned
        #COMMAND ${CMAKE_COMMAND} -E remove coverage.base coverage.total ${PROJECT_BINARY_DIR}/coverage.info.cleaned
        #WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
        #DEPENDS tests
    #)
#endif()
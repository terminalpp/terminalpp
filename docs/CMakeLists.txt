cmake_minimum_required (VERSION 3.5)

find_package(Doxygen)
if (DOXYGEN_FOUND)
    message(STATUS "doxygen found - doxygen target is available")
    # replaces the paths to their proper values
    set(DOXYGEN_IN ${CMAKE_CURRENT_SOURCE_DIR}/doxygen/Doxyfile)
    set(DOXYGEN_OUT ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)
    configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT} @ONLY)

    add_custom_target(doxygen
        COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )
    set_target_properties(doxygen PROPERTIES EXCLUDE_FROM_ALL TRUE)
endif (DOXYGEN_FOUND)

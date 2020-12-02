if(ARCH_LINUX)
    configure_file(${CMAKE_SOURCE_DIR}/tpp-bypass/install.sh ${CMAKE_CURRENT_BINARY_DIR}/tpp-bypass-source/install.sh @ONLY)

    add_custom_target(tpp-bypass-source
        COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_SOURCE_DIR}/stamp.h
            stamp.h
        COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_SOURCE_DIR}/tpp-bypass/main_bypass.cpp
            main_bypass.cpp
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/tpp-bypass-source            
        DEPENDS tpp-bypass
    )
    add_dependencies(packages tpp-bypass-source)

endif()
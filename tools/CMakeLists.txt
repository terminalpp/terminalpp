cmake_minimum_required (VERSION 3.5)

# Simple target that just executes the gitstamp from helpers so that the terminal can automatically have its version set. 
add_executable(
    stamp
	stamp.cpp
)

add_custom_target(tpp-create-stamp
    COMMAND stamp ${CMAKE_SOURCE_DIR} ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION} tpp/stamp.h
)

if(UNIX AND NOT APPLE)
    add_custom_target(install-packages
	    COMMAND sudo apt install libx11-dev libxft-dev
	)
	add_custom_target(install-fonts 
		COMMAND mkdir -p ~/.fonts
        COMMAND cp ${CMAKE_SOURCE_DIR}/resources/fonts/* ~/.fonts/
        COMMAND fc-cache -v
	)
    add_custom_target(install-packages-devel
        COMMAND sudo apt install imagemagick
    )
    add_custom_target(icons 
        COMMAND convert -background transparent logo.png -define icon:auto-resize=16,24,32,48,64,72,96,128,256 tpp.ico
        COMMAND convert tpp.ico -depth 8 tpp.rgba
        COMMAND xIconCpp tppIcon ../tpp/x11/tppIcon.cpp tpp.rgba 16 24 32 64
        COMMAND rm tpp.rgba
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/resources/images
    )
    add_executable(
        xIconCpp EXCLUDE_FROM_ALL
        xIconCpp.cpp
    )
    add_dependencies(icons xIconCpp)
    set_target_properties(icons PROPERTIES EXCLUDE_FROM_ALL TRUE)
    set_target_properties(install-packages PROPERTIES EXCLUDE_FROM_ALL TRUE)
    set_target_properties(install-fonts PROPERTIES EXCLUDE_FROM_ALL TRUE)
    set_target_properties(install-packages-devel PROPERTIES EXCLUDE_FROM_ALL TRUE)
endif()

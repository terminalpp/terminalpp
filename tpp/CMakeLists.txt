# CMakeList.txt : CMake project for t++, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.5)

project(tpp)

include_directories(".")

file(GLOB_RECURSE SRC "*.cpp" "*.h")

if(WIN32)
    add_executable(tpp WIN32 ${SRC} "directwrite/tpp.rc")

    # Search for the wix toolkit    
    file(GLOB WIX_ROOT "C:/Program Files*/WiX Toolset v*/bin")

    if (WIX_ROOT) 
        message(STATUS "WIX toolset found in ${WIX_ROOT}, tpp-msi target is available")
        add_custom_target(tpp-msi
            COMMAND ${CMAKE_COMMAND} -E make_directory "build/installer/msi"
            COMMAND ${WIX_ROOT}/candle.exe
                -nologo 
                -arch "x64" 
                -ext WixUIExtension -ext WixUtilExtension 
                -out "build/installer/msi/tpp.wixobj" 
                "tpp/installer/msi/tpp.wxs"
            COMMAND ${WIX_ROOT}/light.exe 
                -nologo
                -ext WixUIExtension
                -ext WixUtilExtension 
                -out "build/installer/msi/tpp.msi" 
                -sice:ICE61 -sice:ICE91 
                "build/installer/msi/tpp.wixobj"
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        )        
        set_target_properties(tpp-msi PROPERTIES EXCLUDE_FROM_ALL TRUE)
    else()
        message(STATUS "WIX toolset not found, tpp-msi is not available")
    endif()

elseif(UNIX)
    find_package(X11 REQUIRED)
	find_package(Threads REQUIRED)
    find_package(Freetype REQUIRED)
	message(${LUTIL})
    include_directories(${X11_INCLUDE_DIR})
    include_directories(${FREETYPE_INCLUDE_DIRS})
	if (NOT X11_Xft_FOUND)
	    message(FATAL_ERROR "xft not found - please install libxft-dev")
	endif()
    add_executable(tpp ${SRC})
    target_link_libraries(tpp ${X11_LIBRARIES} ${X11_Xft_LIB})  
    target_link_libraries(tpp ${X11_Xft_LIB})  
	target_link_libraries(tpp ${CMAKE_THREAD_LIBS_INIT})
    target_link_libraries(tpp ${FREETYPE_LIBRARIES})
    # install the tpp
    install(TARGETS tpp DESTINATION bin)
else()
    message(FATAL_ERROR "Only Windows and Linux are supported for now")
endif() 

target_link_libraries(tpp libvterm libui)
add_dependencies(tpp tpp-create-stamp)

# Icons
# =====
#
# Creates the appropriate icons for both windows and linux versions. However, since imagemagick is used the target itself is only supported on Linux. 
#
# The generated resources are part of the repository and should only be reran when the original resource files change. 
if(ARCH_LINUX)
    message(STATUS "targets: icons target available for updating terminalpp icons")
    # the xIconCpp is a simple C++ program which takes a RGBA multisize icons and converts it to the format required by the X server, which is then stored as a C++ literal array in specified header file 
    add_executable(
        xIconCpp EXCLUDE_FROM_ALL
        xIconCpp.cpp
    )
    set_target_properties(xIconCpp PROPERTIES EXCLUDE_FROM_ALL TRUE)
    # creates the win32 icon from the logo and the logo with notification, then converts the icons to RGBA a creates the c++ header files containing their contents as literals
    # sizes from the icon exported must be sequential, i.e. all sizes in the icon from the smallest to the largest exported must appear in the as arguments to xIconCpp
    add_custom_target(icons 
        # create icon files for Windows 
        COMMAND convert -background transparent ../images/logo.png -define icon:auto-resize=16,24,32,48,64,72,96,128,256 icon.ico
        COMMAND convert -background transparent ../images/logo-notification.png -define icon:auto-resize=16,24,32,48,64,72,96,128,256 icon-notification.ico
        # create rgba icons for X11
        COMMAND convert icon.ico -depth 8 icon.rgba
        COMMAND xIconCpp tppIcon ${CMAKE_SOURCE_DIR}/terminalpp/x11/resources/tppIcon.cpp icon.rgba 16 24 32 48 64
        COMMAND rm icon.rgba
        COMMAND convert icon-notification.ico -depth 8 icon.rgba
        COMMAND xIconCpp tppIconNotification ${CMAKE_SOURCE_DIR}/terminalpp/x11/resources/tppIconNotification.cpp icon.rgba 16 24 32 48 64
        COMMAND rm icon.rgba
        # create the different sizes of icons used for desktops, qt and so on
        COMMAND convert ../images/logo.png -resize 16x16 icon_16x16.png
        COMMAND convert ../images/logo-notification.png -resize 16x16 icon-notification_16x16.png
        COMMAND convert ../images/logo.png -resize 32x32 icon_32x32.png
        COMMAND convert ../images/logo-notification.png -resize 32x32 icon-notification_32x32.png
        COMMAND convert ../images/logo.png -resize 48x48 icon_48x48.png
        COMMAND convert ../images/logo-notification.png -resize 48x48 icon-notification_48x48.png
        COMMAND convert ../images/logo.png -resize 64x64 icon_64x64.png
        COMMAND convert ../images/logo-notification.png -resize 64x64 icon-notification_64x64.png
        COMMAND convert ../images/logo.png -resize 128x128 icon_128x128.png
        COMMAND convert ../images/logo-notification.png -resize 128x128 icon-notification_128x128.png
        COMMAND convert ../images/logo.png -resize 256x256 icon_256x256.png
        COMMAND convert ../images/logo-notification.png -resize 256x256 icon-notification_256x256.png
        COMMAND convert ../images/logo.png -resize 512x512 icon_512x512.png
        COMMAND convert ../images/logo-notification.png -resize 512x512 icon-notification_512x512.png
        # 1080x1080 box art for windows store
        COMMAND convert ../images/logo.png -resize 1080x1080 icon_1080x1080.png
        # 150x150 and 44x44 icon for msix
        COMMAND convert ../images/logo.png -resize 150x150 icon_150x150.png
        COMMAND convert ../images/logo.png -resize 44x44 icon_44x44.png
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/resources/icons
        DEPENDS xIconCpp
    )
    set_target_properties(icons PROPERTIES EXCLUDE_FROM_ALL TRUE)
endif()

# macOS Bundle Iconsset
#
#
if(ARCH_MACOS)
    add_custom_target(icons
        COMMAND mkdir bundle.iconset
        COMMAND cp icon_16x16.png bundle.iconset/icon_16x16.png
        COMMAND cp icon_32x32.png bundle.iconset/icon_32x32.png
        COMMAND cp icon_128x128.png bundle.iconset/icon_128x128.png
        COMMAND cp icon_256x256.png bundle.iconset/icon_256x256.png
        COMMAND cp icon_512x512.png bundle.iconset/icon_512x512.png
        COMMAND iconutil -c icns bundle.iconset
        COMMAND rm -rf bundle.iconset
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/resources/icons
    )
    set_target_properties(icons PROPERTIES EXCLUDE_FROM_ALL TRUE)
endif()

# SLOC Counter
# ============
#
# A simple counter of lines of code in the entire project. Note that this is only useful if all repos have been downloaded first. 
if(ARCH_UNIX)
    add_custom_target(cloc
        COMMAND cloc helpers ropen terminalpp tests tools tpp-bypass tpp-lib ui ui-terminal
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    )
    set_target_properties(cloc PROPERTIES EXCLUDE_FROM_ALL TRUE)
endif()


#pragma once

namespace tpp {

    char const * JSONSettingsVersion() {
        return PROJECT_VERSION;
    }

    /** Contains the default settings for the terminal as a raw JSON literal so that they can be easily edited. 
     
        Not all settings have to be filled in, as once the default JSON is read, it is updated to the actual platform & installation in the configuration. 
     */
    char const * DefaultJSONSettings() {
        return R"json(
/* Default settings for tpp 
 */
{  
    /* The version of tpp the settings are intended for. For debugging purposes only.
     */
    "version" : "",

    /* Log properties.
     */
    "log" : {
        /* File to which all terminal input should be logged.
         */
        "file" : ""
    },
    
    /* Renderer settings. 
     */
    "renderer" : {
        /* Maximum FPS. 
         */
        "fps" : 60
    },

    /* The font used to redner the terminal and its size.
     */
    "font" : {
        "family" : "",
        "doubleWidthFamily" : "",
        "size" : 18
    },

    /* Session properties. 
     */
    "session" : {
        /* Determines whether local, or bypass PTY should be used. Useful only for Windows, ignored on other systems. 
         */
        "pty" : "",
        /* The command to execute in the session. 
         */
        "command" : [  ],
        /* Number of columns the non-maximized window should have. 
         */
        "cols" : 80,
        /* Number of rows the non-maximized window whould have.
         */
        "rows" : 25,
        /* Determines whether the window should start fullscreen or not.
         */
        "fullscreen" : false,
        /* Determines the maximum number of lines the terminal will remember in the history of the buffer. If set to 0, terminal history is disabled. 
         */
        "historyLimit" : 10000,
        /* Definition of the palette used for the session.
         */
        "palette" : {
            /* Overrides the predefined palette. Up to 256 colors can be specified in HTML format. These colors will override the default xterm palette used. 
             */
            "colors" : [],
            /* Specifies the index of the default foreground color in the palette. 
             */
            "defaultForeground" : 15,
            /* Specifies the index of the default background color in the palette.
             */
            "defaultBackground" : 0
        },
        /* Settings for opening remote files from the terminal locally. 
         */
        "remoteFiles" : {
            /* Directory to which the remote files should be downloaded. If empty, temporary directory will be used. 
             */
            "dir" : ""
        }
    }
}
        )json";
    }



} // namespace tpp



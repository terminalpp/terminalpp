#pragma once

namespace tpp {

    char const * JSONSettingsVersion() {
        return "0.22";
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
        "family" : "Iosevka Term",
        "doubleWidthFamily" : "SpaceMono NF",
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
        "fullscreen" : false
    }
}
        )json";
    }



} // namespace tpp



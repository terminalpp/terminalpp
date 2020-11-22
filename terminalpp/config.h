#pragma once

#include <vector>

#include "helpers/process.h"
#include "helpers/json_config.h"
#include "helpers/telemetry.h"
#include "helpers/version.h"

#include "ui/color.h"
#include "ui/canvas.h"
#include "ui-terminal/ansi_terminal.h"
#include "ui/special_objects/hyperlink.h"

/** \page tppconfig Configuration
 
    \brief Describes the confifuration of the terminal application.

	What I want:

	- configuration stored in JSON (not all settings must be stored)
	- typechecked reading of the configuration in the app = must be static, need macros to specify
	- one place to define the options and their defaults
 */

/** The oldest compatible settings version. 
 
    If upgrading terminal from version above or equal to the one set here, the upgrade is silent because the configuration files should be almost identical (i.e. new version should have only additions).

    In other cases a version upgrade dialog is displayed and a copy of the settings should be made before terminal updates to new version. 
    */
#define MIN_COMPATIBLE_VERSION "0.8.0"

#define BYPASS_FOLDER "~/.local/bin"
#define BYPASS_PATH "~/.local/bin/tpp-bypass"

#define DEFAULT_WINDOW_TITLE "t++"

/** Determines the default blink speed of the cursor or blinking text. 
 */ 
#define DEFAULT_BLINK_SPEED 500

/** Keyboard shortcuts for various actions.
 */
#define SHORTCUT_FULLSCREEN (Key::Enter + Key::Alt)
#define SHORTCUT_ABOUT (Key::F1 + Key::Alt)
#define SHORTCUT_SETTINGS (Key::F10 + Key::Alt)
#define SHORTCUT_ZOOM_IN (Key::Equals + Key::Ctrl)
#define SHORTCUT_ZOOM_OUT (Key::Minus + Key::Ctrl)
// alternate zoom shortcuts like in browsers
#define SHORTCUT_ZOOM_IN_ALT (Key::Equals + Key::Ctrl + Key::Shift)
#define SHORTCUT_ZOOM_OUT_ALT (Key::Minus + Key::Ctrl + Key::Shift)

#define SHORTCUT_PASTE (Key::V + Key::Ctrl + Key::Shift)
#define SHORTCUT_COPY (Key::C + Key::Ctrl)


namespace config {

    /** Typedef for font attributes specification only. 
     */
    typedef ui::Font FontAttributes;

    enum class ConfirmPaste {
        Never,
        Always,
        Multiline
    };

    /** Determines whether the terminal applications can set local clipboard. 
     
        If allowed, all requests to set clipboard will be processed immediately. If denied, all will be silently ignored and when set to ask, each request will require confirmation.
     */
    enum class AllowClipboardUpdate {
        Allow,
        Deny,
        Ask
    };

}

template<>
inline Version JSONConfig::FromJSON(JSON const & json) {
    if (json.kind() != JSON::Kind::String)
        THROW(JSONError()) << "Element must be a string";
    return Version{json.toString()};
}

template<>
inline config::FontAttributes JSONConfig::FromJSON(JSON const & json) {
    if (json.kind() != JSON::Kind::String)
        THROW(JSONError()) << "Element must be a string";
    config::FontAttributes result;
    for (std::string const & item : SplitAndTrim(ToLower(json.toString()), " ")) {
        if (item == "bold") {
            result.setBold();
        } else if (item == "italic") {
            result.setItalic();
        } else if (item == "underline") {
            result.setUnderline();
        } else if (item == "strikethrough") {
            result.setStrikethrough();
        } else if (item == "dashed") {
            result.setDashed();
        } else if (item == "blink") {
            result.setBlink();
        } else {
            THROW(JSONError()) << "Unknown font attribute " << item << " found";
        }
    }
    return result;
}

template<>
inline Command JSONConfig::FromJSON(JSON const & json) {
    if (json.kind() != JSON::Kind::Array)
        THROW(JSONError()) << "Element must be an array";
    std::vector<std::string> cmd;
    for (auto i : json) {
        if (i.kind() != JSON::Kind::String) 
            THROW(JSONError()) << "Element items must be strings, but " << i.kind() << " found";
        cmd.push_back(i.toString());
    }
    return Command{cmd};
}

template<>
inline void JSONConfig::Property<Command>::cmdArgUpdate(char const * value, size_t index) {
    JSON x = (index == 0) ? JSON::Array() : toJSON(false);
    x.add(JSON{value});
    update(x, [](JSONError &&e) { throw std::move(e);});
}

template<>
inline ui::AnsiTerminal::Palette JSONConfig::FromJSON(JSON const & json) {
    if (json.kind() != JSON::Kind::Array)
        THROW(JSONError()) << "Element must be an array";
    ui::AnsiTerminal::Palette result{ui::AnsiTerminal::Palette::XTerm256()};
    size_t i = 0;
    for (auto c : json) {
        if (c.kind() != JSON::Kind::String) 
            THROW(JSONError()) << "Element items must be HTML colors, but " << c.kind() << " found";
        // empty colors are skipped in the color configuration, leaving their default values
        if (! c.toString().empty())
            result[i] = ui::Color::FromHTML(c.toString());
        ++i;
    }
    return result;
}

template<>
inline ui::Color JSONConfig::FromJSON(JSON const & json) {
    if (json.kind() != JSON::Kind::String)
        THROW(JSONError()) << "Element must be an array";
    return ui::Color::FromHTML(json.toString());
}

template<>
inline std::vector<std::reference_wrapper<Log>> JSONConfig::FromJSON(JSON const & json) {
    std::vector<std::reference_wrapper<Log>> result;
    if (json.kind() != JSON::Kind::Array)
        THROW(JSONError()) << "Expected array, but " << json << " found";
    for (auto & item : json) {
        if (item.kind() != JSON::Kind::String)
            THROW(JSONError()) << "Strings expected in the array, but  " << item << " found";
        std::string const & logName = item.toString();
        if (logName == "FATAL_ERROR") 
            result.push_back(Telemetry::FatalErrorLog());
        else if (logName == "EXCEPTION") 
            result.push_back(Log::Exception());
        else if (logName == "TELEMETRY") 
            result.push_back(Telemetry::TelemetryLog());
        else if (logName == "SEQ_ERROR") 
            result.push_back(ui::AnsiTerminal::SEQ_ERROR);
        else if (logName == "SEQ_UNKNOWN") 
            result.push_back(ui::AnsiTerminal::SEQ_UNKNOWN);
        else if (logName == "SEQ_WONT_SUPPORT") 
            result.push_back(ui::AnsiTerminal::SEQ_WONT_SUPPORT);
        else
            THROW(JSONError()) << "Invalid log name " << logName;
    }
    return result;
}

template<>
inline config::ConfirmPaste JSONConfig::FromJSON(JSON const & json) {
    if (json.kind() != JSON::Kind::String)
        THROW(JSONError()) << "Element must be an array";
    if (json.toString() == "never") 
        return config::ConfirmPaste::Never;
    else if (json.toString() == "always")
        return config::ConfirmPaste::Always;
    else if (json.toString() == "multiline")
        return config::ConfirmPaste::Multiline;
    else 
        THROW(JSONError()) << "Only values 'never', 'always' or 'multiline' are permitted";
}

template<>
inline config::AllowClipboardUpdate JSONConfig::FromJSON(JSON const & json) {
    if (json.kind() != JSON::Kind::String)
        THROW(JSONError()) << "Element must be an array";
    if (json.toString() == "allow") 
        return config::AllowClipboardUpdate::Allow;
    else if (json.toString() == "deny")
        return config::AllowClipboardUpdate::Deny;
    else if (json.toString() == "ask")
        return config::AllowClipboardUpdate::Ask;
    else 
        THROW(JSONError()) << "Only values 'allow', 'deny' or 'ask' are permitted";
}

namespace tpp {	

    class Config : public JSONConfig::CmdArgsRoot {
    public:

        CONFIG_OBJECT(
            version,
            "Version information & checks",
            CONFIG_PROPERTY(
                version,
                "Version of tpp the settings are intended for, to make sure the settings are useful and to detect version changes",
                TerminalVersion,
                Version
            );
            CONFIG_PROPERTY(
                checkChannel,
                "Release channel to be checked for new version upon start. Leave empty (default) if the check should not be performed.",
                JSON{""},
                std::string
            );
        );
        CONFIG_OBJECT(
            application,
            "Application specific settings",
            CONFIG_PROPERTY(
                detectSessionsAtStartup,
                "If true, checks that profile shortcuts (if supported on given platform) will be updated at every startup",
                JSON{true},
                bool
            );
        );
        CONFIG_OBJECT(
            telemetry,
            "Telemetry Settings for bug and feature requests reporting",
            CONFIG_PROPERTY(
                dir,
                "Directory where to store the telemetry logs",
                DefaultTelemetryDir,
                std::string
            );
            CONFIG_PROPERTY(
                deleteAtExit,
                "If true, unused telemetry logs are deleted when the application terminates",
                JSON{true},
                bool
            );
            CONFIG_PROPERTY(
                events,
                "Names of event kinds that should be captured by the telemetry",
                JSON::Array(),
                std::vector<std::reference_wrapper<Log>>
            );
        );
        CONFIG_OBJECT(
            renderer,
            "Renderer settings",
		    CONFIG_PROPERTY(
				fps, 
				"Maximum FPS", 
				JSON{60},
			    unsigned
			);
            CONFIG_OBJECT(
                hyperlinks,
                "Settings for displaying hyperlinks",
                CONFIG_OBJECT(
                    normal,
                    "Inactive hyperlink (detected or explicit)",
                    CONFIG_PROPERTY(
                        foreground,
                        "Foreground color of the hyperlink (blended over existing)",
                        JSON{"#00000000"},
                        ui::Color
                    );
                    CONFIG_PROPERTY(
                        background,
                        "Foreground color of the hyperlink (blended over existing)",
                        JSON{"#00000000"},
                        ui::Color
                    );
                    CONFIG_PROPERTY(
                        font,
                        "Font attributes of the hyperlink, space separated 'underline', 'dashed', 'italic' and 'bold' are supported",
                        JSON{"underline dashed"},
                        config::FontAttributes
                    );
                    /** Returns the actual hyperlink style in single object.
                     */
                    ui::Hyperlink::Style operator () () const {
                        return ui::Hyperlink::Style{foreground(), background(), font()};
                    }
                );
                CONFIG_OBJECT(
                    active,
                    "Active (mouse over) hyperlink (detected or explicit)",
                    CONFIG_PROPERTY(
                        foreground,
                        "Foreground color of the hyperlink (blended over existing)",
                        JSON{"#0000ff"},
                        ui::Color
                    );
                    CONFIG_PROPERTY(
                        background,
                        "Foreground color of the hyperlink (blended over existing)",
                        JSON{"#00000000"},
                        ui::Color
                    );
                    CONFIG_PROPERTY(
                        font,
                        "Font attributes of the hyperlink, space separated 'underline', 'dashed', 'italic' and 'bold' are supported",
                        JSON{"underline dashed"},
                        config::FontAttributes
                    );
                    /** Returns the actual hyperlink style in single object.
                     */
                    ui::Hyperlink::Style operator () () const {
                        return ui::Hyperlink::Style{foreground(), background(), font()};
                    }
                );
            );
            CONFIG_OBJECT(
                font,
                "Font used to render the terminal",
                CONFIG_PROPERTY(
                    family,
                    "Font to render default size characters",
                    DefaultFontFamily,
                    std::string
                );
                CONFIG_PROPERTY(
                    boldFamily,
                    "Font to render bold characters, if different from normal font",
                    JSON{""},
                    std::string
                );
                CONFIG_PROPERTY(
                    doubleWidthFamily,
                    "Font to render double width characters",
                    DefaultDoubleWidthFontFamily,
                    std::string
                );
                CONFIG_PROPERTY(
                    doubleWidthBoldFamily,
                    "Font to render bold double width characters, if different from doubleWidth font",
                    JSON{""},
                    std::string
                );
                CONFIG_PROPERTY(
                    size,
                    "Size of the font in pixels at zoom level 1.0",
                    JSON{18},
                    unsigned
                );
                CONFIG_PROPERTY(
                    charSpacing,
                    "Spacing between characters.",
                    JSON{1.0},
                    double
                );
                CONFIG_PROPERTY(
                    lineSpacing,
                    "Spacing between lines.",
                    JSON{1.0},
                    double
                );

            );
            CONFIG_OBJECT(
                window,
                "Properties of the terminal window",
                CONFIG_PROPERTY(
                    cols,
                    "Number of columns the non-maximized window should have.",
                    JSON{80},
                    unsigned
                );
                CONFIG_PROPERTY(
                    rows,
                    "Number of rows the non-maximized window should have.",
                    JSON{25},
                    unsigned
                );
                CONFIG_PROPERTY(
                    fullscreen,
                    "Determines the behavior of the session when the attached command terminates.",
                    JSON{false},
                    bool
                );
                CONFIG_PROPERTY(
                    waitAfterPtyTerminated,
                    "Determines the behavior of the session when the attached command terminates.",
                    JSON{false},
                    bool
                );
                CONFIG_PROPERTY(
                    historyLimit,
                    "Determines the maximum number of lines the terminal will remember in the history of the buffer. If set to 0, terminal history is disabled.",
                    JSON{10000},
                    int
                );
            );
        );
        CONFIG_OBJECT(
            sequences,
            "Behavior customization for terminal escape sequences (VT100)",
			CONFIG_PROPERTY(
				confirmPaste,
				"Determines whether pasting into terminal should be explicitly confirmed. Allowed values are 'never', 'always', 'multiline'.",
				JSON{"multiline"},
			    config::ConfirmPaste
			);
            CONFIG_PROPERTY(
                allowClipboardUpdate,
                "Determines whether terminal applications can set local clipboard. Allowed values are 'allow', 'deny' and 'ask'",
                JSON{"allow"},
                config::AllowClipboardUpdate
            );
            CONFIG_PROPERTY(
                boldIsBright,
                "If true, bold text is rendered in bright colors.",
                JSON{true},
                bool
            );
            CONFIG_PROPERTY(
                displayBold,
                "If true bold font will be used when appropriate.",
                JSON{true},
                bool                
            );
            CONFIG_PROPERTY(
                allowOSCHyperlinks,
                "If true, explicit hyperlink commands (OSC 8) will be displayed as hyperlinks.",
                JSON{true},
                bool
            );
            CONFIG_PROPERTY(
                detectHyperlinks,
                "If true, hyperlinks (http and https) contained within the terminal will be detected and displayed as hyperlinks.",
                JSON{true},
                bool
            );
            CONFIG_PROPERTY(
                allowCursorChanges,
                "If true, terminal applications can change the cursor properties (color, character, etc.)",
                JSON{true},
                bool
            );
        );
        CONFIG_OBJECT(
            remoteFiles,
            "Settings for opening remote files from the terminal locally.",
            CONFIG_PROPERTY(
                dir,
                "Directory to which the remote files should be downloaded. If empty, temporary directory will be used.",
                DefaultRemoteFilesDir,
                std::string
            );
        );
        CONFIG_OBJECT(
            sessionDefaults,
            "Default values for session properties. These will be used when a session does not override the values",
			CONFIG_PROPERTY(
				pty,
				"Determines whether local, or bypass PTY should be used. Useful only for Windows, ignored on other systems.",
				JSON{"local"},
			    std::string
			);
			CONFIG_OBJECT(
				palette,
				"Definition of the palette used for the session.",
				CONFIG_PROPERTY(
					colors, 
					"Overrides the predefined palette. Up to 256 colors can be specified in HTML format. These colors will override the default xterm palette used.",
					JSON::Array(),
				    ui::AnsiTerminal::Palette
				);
				CONFIG_PROPERTY(
				    defaultForeground,
					"Specifies the index of the default foreground color in the palette.",
					JSON{"#ffffff"},
				    ui::Color
				);
				CONFIG_PROPERTY(
					defaultBackground,
					"Specifies the index of the default background color in the palette.",
					JSON{"#000000"},
				    ui::Color
				);
				/** Provides a value getter on the entire palette configuration group which returns the palette with the default colors set accordingly. 
				 */
				ui::AnsiTerminal::Palette * operator () () const {
					auto result = new ui::AnsiTerminal::Palette{colors()};
					result->setDefaultForeground(defaultForeground());
					result->setDefaultBackground(defaultBackground());
					return result;
				}
			);
			CONFIG_OBJECT(
				cursor,
			    "Cursor properties",
                CONFIG_PROPERTY(
                    codepoint,
                    "UTF codepoint of the cursor",
                    JSON{0x2581},
                    unsigned
                );
                CONFIG_PROPERTY(
                    color,
                    "Color of the cursor",
                    JSON{"#ffffff"},
                    ui::Color
                );
                CONFIG_PROPERTY(
                    blink,
                    "Determines whether the cursor blinks or not.",
                    JSON{true},
                    bool
                );
                CONFIG_PROPERTY(
                    inactiveColor,
                    "Color of the rectangle showing the cursor position when not focused.",
                    JSON{"#00ff00"},
                    ui::Color
                );
				/** Value getter for cursor properies aggregated in a ui::Cursor object.
				 */
        		ui::Canvas::Cursor operator () () const {
					ui::Canvas::Cursor result{};
					result.setVisible(true);
					result.setCodepoint(codepoint());
					result.setColor(color());
					result.setBlink(blink());
					return result;
				}
			);

        );
        CONFIG_PROPERTY(
            defaultSession,
            "Name of the default session which will be opened when terminal starts",
            JSON{"default"},
            std::string
        );
        /* The list of sessings and their override settings. 
         */
        CONFIG_ARRAY(
            sessions, 
            "List of known sessions",
            JSON::Array(),
            CONFIG_PROPERTY(
                name,
                "Name of the session",
                JSON{""},
                std::string
            );
            CONFIG_PROPERTY(
                hidden,
                "Can hide the session from menus, such as the jumplist. Hidden session can still be explicitly started via the --session argument",
                JSON{false},
                bool
            );
			CONFIG_PROPERTY(
				pty,
				"Determines whether local, or bypass PTY should be used. Useful only for Windows, ignored on other systems.",
				JSON{"local"},
			    std::string
			);
			CONFIG_PROPERTY(
				command, 
				"The command to be executed in the session",
				JSON::Array(),
			    Command
			);
            CONFIG_PROPERTY(
                workingDirectory,
                "Where the terminal session should be launched, empty to use current working directory",
                JSON{""},
                std::string
            );
			CONFIG_OBJECT(
				palette,
				"Definition of the palette used for the session.",
				CONFIG_PROPERTY(
					colors, 
					"Overrides the predefined palette. Up to 256 colors can be specified in HTML format. These colors will override the default xterm palette used.",
					JSON::Array(),
				    ui::AnsiTerminal::Palette
				);
				CONFIG_PROPERTY(
				    defaultForeground,
					"Specifies the index of the default foreground color in the palette.",
					JSON{"#ffffff"},
				    ui::Color
				);
				CONFIG_PROPERTY(
					defaultBackground,
					"Specifies the index of the default background color in the palette.",
					JSON{"#000000"},
				    ui::Color
				);
				/** Provides a value getter on the entire palette configuration group which returns the palette with the default colors set accordingly. 
				 */
				ui::AnsiTerminal::Palette * operator () () const {
					auto result = new ui::AnsiTerminal::Palette{colors()};
					result->setDefaultForeground(defaultForeground());
					result->setDefaultBackground(defaultBackground());
					return result;
				}
			);
			CONFIG_OBJECT(
				cursor,
			    "Cursor properties",
                CONFIG_PROPERTY(
                    codepoint,
                    "UTF codepoint of the cursor",
                    JSON{0x2581},
                    unsigned
                );
                CONFIG_PROPERTY(
                    color,
                    "Color of the cursor",
                    JSON{"#ffffff"},
                    ui::Color
                );
                CONFIG_PROPERTY(
                    blink,
                    "Determines whether the cursor blinks or not.",
                    JSON{true},
                    bool
                );
                CONFIG_PROPERTY(
                    inactiveColor,
                    "Color of the rectangle showing the cursor position when not focused.",
                    JSON{"#00ff00"},
                    ui::Color
                );
				/** Value getter for cursor properies aggregated in a ui::Cursor object.
				 */
				ui::Canvas::Cursor operator () () const {
					ui::Canvas::Cursor result{};
					result.setVisible(true);
					result.setCodepoint(codepoint());
					result.setColor(color());
					result.setBlink(blink());
					return result;
				}
			);
        );

        /** Initializes the configuration. 
         
            Reads the configuration if one exists. Then fills in missing values and updates the stored settings if necessary. If there are any errors with reading the settings, creates a backup of the old settings before creating new ones. 

            Also checks the stored version and current version and informs the user if there is a possibility of any breaking change. 
         */
        static Config & Setup(int argc, char* argv[]);

        static Config & Instance() {
            static Config instance;
            return instance;
        }

		/** Returns the directory in which the configuration files should be located. 
		 */
	    static std::string GetSettingsFolder();

		/** Returns the actual settings file, i.e. the `settings.json` file in the settings directory. 
		 */
	    static std::string GetSettingsFile();

        /** Returns the version of the terminal++ binary. 
		 
		    This version is specified in the CMakeLists.txt and is watermarked to each binary.
		 */
	    static JSON TerminalVersion();

        sessions_entry & sessionByName(std::string const & sessionName) {
            for (size_t i = 0, e = sessions.size(); i < e; ++i)
                if (sessions[i].name() == sessionName)
                    return sessions[i];
            THROW(Exception()) << "Session " << sessionName << " not found";
        }

        sessions_entry const & sessionByName(std::string const & sessionName) const {
            for (size_t i = 0, e = sessions.size(); i < e; ++i)
                if (sessions[i].name() == sessionName)
                    return sessions[i];
            THROW(Exception()) << "Session " << sessionName << " not found";
        }

        std::string familyForFont(ui::Font font) const {
            if (font.doubleWidth()) {
                if (font.bold() && renderer.font.doubleWidthBoldFamily.updated())
                    return renderer.font.doubleWidthBoldFamily();
                else   
                    return renderer.font.doubleWidthFamily();
            } else {
                if (font.bold() && renderer.font.boldFamily.updated())
                    return renderer.font.boldFamily();
                else   
                    return renderer.font.family();
            }
        }

    protected:
        /** Parses the command line arguments. 
            
         */
        void parseCommandLine(int argc, char * argv[]) {
            addArgument(renderer.fps, { "--fps"});
            addArgument(renderer.font.family, {"--font"});
            addArgument(renderer.font.size, {"--font-size"});
            addArgument(renderer.window.cols, {"--cols", "-c"});
            addArgument(renderer.window.rows, {"--rows", "-r"});
            //addArgument(application.useCwdForSessions, {"-cwd"}, "true");
            addArgument(defaultSession, {"--session"});
            // create new empty session that we use to store the pty and command arguments so that they are matched the same way
            sessions_entry & cmdSession = sessions.addElement(JSON::Object());
            addArgument(cmdSession.command, {"-e"});
            setLastArgument(cmdSession.command);
            addArgument(cmdSession.pty, {"--pty"});
            addArgument(cmdSession.workingDirectory, {"--here"}, "");
            // parse the arguments
            CmdArgsRoot::parseCommandLine(argc, argv);
            // if any of the session arguments are overriden, create proper session this time, initialize it with default session (now reflecting the --session argument value, if any)
            // this is by no means efficient, but since its done only once at startup, we don't really care
            if (cmdSession.pty.updated() || cmdSession.command.updated() || cmdSession.workingDirectory.updated()) {
                sessions_entry & base = sessionByName(defaultSession());
                sessions_entry & session = sessions.addElement(base.toJSON());
                // copy base to the session and make sure it won't appear menus
                session.hidden.set(JSON{true});
                // if the pty was explicitly updated, propagate to the new session, otherwise reset whatever pty was there to local
                if (cmdSession.pty.updated())
                    session.pty.set(cmdSession.pty.toJSON());
                else if (cmdSession.command.updated())
                    session.pty.set(JSON{"local"});
                // copy the changed command line values
                if (cmdSession.command.updated())
                    session.command.set(cmdSession.command.toJSON());
                // copy the working directory
                if (cmdSession.workingDirectory.updated())
                    session.workingDirectory.set(cmdSession.workingDirectory.toJSON());
                // set the session name and set it as default session 
                JSON name{"command-line-override"};
                defaultSession.set(name);
                session.name.set(name);
            } 
            sessions.erase(cmdSession);
        }

    private:

        /** Verifies the configuration version stored in the settings. 
         
            If the version is different than current version of the program, clears the version info so that it gets regenerated. If the old version is lower than the MIN_COMPATIBLE_VERSION, displays a warning that settings will be updated. 
         */
        static void VerifyConfigurationVersion(JSON & userConfig);

        /** Patches the sessions list with autodetected sessions.
         */
        bool patchSessions();

        bool addSession(JSON const & session);

		/** \name Default value providers
		 
		    These static methods calculate default values for the complex configuration properties. The idea is that these will be executed once the terminal is installed, they will analyze the system and calculate proper values to be stored in the configuration file. 
		 */
		//@{

		
		static JSON DefaultTelemetryDir();

		static JSON DefaultRemoteFilesDir();		

		static JSON DefaultFontFamily();

		static JSON DefaultDoubleWidthFontFamily();

        //static JSON DefaultSessions();

        //@}


    #if (defined ARCH_WINDOWS)

        static bool WSLIsBypassPresent(std::string const & distro);

        static bool WSLInstallBypass(std::string const & distro);

        /** Adds the cmd.exe session to the list of sessions and sets it as default. 
         
            `cmd.exe` is expected to be installed on every Windows computer so is added without check. 
         */
        void win32AddCmdExe(std::string & defaultSessionName, bool & updated);

        /** Adds the powershell session to the list of sessions and sets it as default. 
         
            `powershell` is expected to be installed on every Windows computer so is added without check. 
         */
        void win32AddPowershell(std::string & defaultSessionName, bool & updated);

        /** Adds sessions for WSL distributions and sets the default WSL distro as default session. 
         
            The presence of WSL and its distributions and their names is checked as it's an optional feature. 
         */
        void win32AddWSL(std::string & defaultSessionName, bool & updated);

        /** Adds sessions for msys2, if found.
         
            Does not set msys2 as the default session. The msys2 session specifications are taken from  https://www.msys2.org/docs/terminals/.
         */
        void win32AddMsys2(std::string & defaultSessionName, bool & updated);

    #endif

    }; // tpp::Config

} // namespace tpp

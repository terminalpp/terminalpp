#pragma once

#include <vector>

#include "helpers/process.h"
#include "helpers/json_config.h"
#include "helpers/telemetry.h"

#include "ui-terminal/ansi_terminal.h"

/** \page tppconfig Configuration
 
    \brief Describes the confifuration of the terminal application.

	What I want:

	- configuration stored in JSON (not all settings must be stored)
	- typechecked reading of the configuration in the app = must be static, need macros to specify
	- one place to define the options and their defaults
 */

//#define SHOW_LINE_ENDINGS

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

#ifdef FOOBAR

template<>
inline Command JSONConfig::ParseValue(JSON const & json) {
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
inline ui::AnsiTerminal::Palette JSONConfig::ParseValue(JSON const & json) {
    if (json.kind() != JSON::Kind::Array)
        THROW(JSONError()) << "Element must be an array";
    ui::AnsiTerminal::Palette result{ui::AnsiTerminal::Palette::XTerm256()};
    size_t i = 0;
    for (auto c : json) {
        if (c.kind() != JSON::Kind::String) 
            THROW(JSONError()) << "Element items must be HTML colors, but " << c.kind() << " found";
        result[i] = ui::Color::FromHTML(c.toString());
    }
    return result;
}

template<>
inline ui::Color JSONConfig::ParseValue(JSON const & json) {
    if (json.kind() != JSON::Kind::String)
        THROW(JSONError()) << "Element must be an array";
    return ui::Color::FromHTML(json.toString());
}

template<>
inline std::vector<std::reference_wrapper<Log>> JSONConfig::ParseValue(JSON const & json) {
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

/** Parts of the command argument are just JSON strings so they are only surrounded by double quotes. 
 */
template<>
inline std::string JSONArguments::ConvertToJSON<Command>(std::string const & value) {
    return STR("\"" << value << "\"");
}

#endif // FOOBAR

namespace x {

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
inline ui::AnsiTerminal::Palette JSONConfig::FromJSON(JSON const & json) {
    if (json.kind() != JSON::Kind::Array)
        THROW(JSONError()) << "Element must be an array";
    ui::AnsiTerminal::Palette result{ui::AnsiTerminal::Palette::XTerm256()};
    size_t i = 0;
    for (auto c : json) {
        if (c.kind() != JSON::Kind::String) 
            THROW(JSONError()) << "Element items must be HTML colors, but " << c.kind() << " found";
        result[i] = ui::Color::FromHTML(c.toString());
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

}

namespace tpp {	

    class Config : public x::JSONConfig::Root {
    public:
        CONFIG_OBJECT(
            version,
            "Version information & checks",
            CONFIG_PROPERTY(
                version,
                "Version of tpp the settings are intended for, to make sure the settings are useful and to detect version changes",
                JSON{""},
                std::string
            );
            CONFIG_PROPERTY(
                checkChannel,
                "Release channel to be checked for new version upon start. Leave empty (default) if the check should not be performed.",
                JSON{""},
                std::string
            );
        );
        CONFIG_OBJECT(
            telemetry,
            "Telemetry Settings for bug and feature requests reporting",
            CONFIG_PROPERTY(
                dir,
                "Directory where to store the telemetry logs",
                JSON{""},
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
                font,
                "Font used to render the terminal",
                CONFIG_PROPERTY(
                    family,
                    "Font to render default size characters",
                    JSON{""},
                    std::string
                );
                CONFIG_PROPERTY(
                    doubleWidthFamily,
                    "Font to render double width characters",
                    JSON{""},
                    std::string
                );
                CONFIG_PROPERTY(
                    size,
                    "Size of the font in pixels at zoom level 1.0",
                    JSON{18},
                    unsigned
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
            // TODO this should be enum so that the type is meaningful for metadata generation, such as schema
			CONFIG_PROPERTY(
				confirmPaste,
				"Determines whether pasting into terminal should be explicitly confirmed. Allowed values are 'never', 'always', 'multiline'.",
				JSON{"multiline"},
			    std::string
			);
            CONFIG_PROPERTY(
                boldIsBright,
                "If true, bold text is rendered in bright colors.",
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
                JSON{""},
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
			CONFIG_PROPERTY(
				command, 
				"The command to be executed in the session",
				JSON::Array(),
			    Command
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
					JSON{15},
				    size_t
				);
				CONFIG_PROPERTY(
					defaultBackground,
					"Specifies the index of the default background color in the palette.",
					JSON{0},
				    size_t
				);
				/** Provides a value getter on the entire palette configuration group which returns the palette with the default colors set accordingly. 
				 */
				ui::AnsiTerminal::Palette operator () () const {
					ui::AnsiTerminal::Palette result{colors()};
					result.setDefaultForegroundIndex(defaultForeground());
					result.setDefaultBackgroundIndex(defaultBackground());
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
        		ui::Cursor operator () () const {
					ui::Cursor result{};
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
					JSON{15},
				    size_t
				);
				CONFIG_PROPERTY(
					defaultBackground,
					"Specifies the index of the default background color in the palette.",
					JSON{0},
				    size_t
				);
				/** Provides a value getter on the entire palette configuration group which returns the palette with the default colors set accordingly. 
				 */
				ui::AnsiTerminal::Palette operator () () const {
					ui::AnsiTerminal::Palette result{colors()};
					result.setDefaultForegroundIndex(defaultForeground());
					result.setDefaultBackgroundIndex(defaultBackground());
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
				ui::Cursor operator () () const {
					ui::Cursor result{};
					result.setVisible(true);
					result.setCodepoint(codepoint());
					result.setColor(color());
					result.setBlink(blink());
					return result;
				}
			);
        );



        Config(JSON const & from):
            Root{from} {
        }

        static Config const & Setup(int argc, char* argv[]) {
            MARK_AS_UNUSED(argc);
            MARK_AS_UNUSED(argv);

            return Config{JSON::Object()};
        }

        static Config const & Instance() {
            NOT_IMPLEMENTED;
        }

		/** Returns the directory in which the configuration files should be located. 
		 */
	    static std::string GetSettingsFolder();

		/** Returns the actual settings file, i.e. the `settings.json` file in the settings directory. 
		 */
	    static std::string GetSettingsFile();


    }; // tpp::Config


#ifdef FOOBAR

	class Config : public JSONConfig::Root {
	public:
	    CONFIG_OPTION(
			version,
			"Version of tpp the settings are intended for, to make sure the settings are useful and to detect version changes",
			TerminalVersion,
			std::string			
		);
        CONFIG_OPTION(
            versionCheckChannel,
            "Release channel to be checked for new version upon start. Leave empty (default) if the check should not be performed.",
            "\"\"",
            std::string
        );

        CONFIG_GROUP(
            telemetry,
            "Telemetry Settings for bug and feature requests reporting",
            CONFIG_OPTION(
                dir,
                "Directory where to store the telemetry logs",
                DefaultTelemetryDir,
                std::string
            );
            CONFIG_OPTION(
                deleteAtExit,
                "If true, unused telemetry logs are deleted when the application terminates",
                "true",
                bool
            );
            CONFIG_OPTION(
                events,
                "Names of event kinds that should be captured by the telemetry",
                "[]",
                std::vector<std::reference_wrapper<Log>>
            );
        );

		CONFIG_GROUP(
			renderer, 
		    "Renderer Settings", 
		    CONFIG_OPTION(
				fps, 
				"Maximum FPS", 
				"60",
			    unsigned
			);
		);

		CONFIG_GROUP(
			font,
		    "Font used to render the terminal",
			CONFIG_OPTION(
				family,
				"Font to render default size characters",
				DefaultFontFamily,
			    std::string
			);
			CONFIG_OPTION(
				doubleWidthFamily,
				"Font to render double width characters",
				DefaultDoubleWidthFontFamily,
			    std::string
			);
			CONFIG_OPTION(
				size,
				"Size of the font in pixels at zoom level 1.0",
				"18",
			    unsigned
			);
		);

		CONFIG_GROUP(
			session,
		    "Session properties",
			// TODO perhaps PTY should be not required and default should be conpty
			CONFIG_OPTION(
				pty,
				"Determines whether local, or bypass PTY should be used. Useful only for Windows, ignored on other systems.",
				DefaultSessionPTY,
			    std::string
			);
			CONFIG_OPTION(
				command, 
				"The command to be executed in the session",
				DefaultSessionCommand,
			    Command
			);
			CONFIG_OPTION(
				cols,
				"Number of columns the non-maximized window should have.",
				"80",
			    unsigned
		    );
			CONFIG_OPTION(
				rows,
				"Number of rows the non-maximized window should have.",
				"25",
			    unsigned
		    );
			CONFIG_OPTION(
				fullscreen,
				"Determines the behavior of the session when the attached command terminates.",
				"false",
			    bool
			);
			CONFIG_OPTION(
				waitAfterPtyTerminated,
				"Determines the behavior of the session when the attached command terminates.",
				"false",
			    bool
			);
			CONFIG_OPTION(
				historyLimit,
				"Determines the maximum number of lines the terminal will remember in the history of the buffer. If set to 0, terminal history is disabled.",
				"10000",
			    int
			);
			CONFIG_GROUP(
				palette,
				"Definition of the palette used for the session.",
				CONFIG_OPTION(
					colors, 
					"Overrides the predefined palette. Up to 256 colors can be specified in HTML format. These colors will override the default xterm palette used.",
					"[]",
				    ui::AnsiTerminal::Palette
				);
				CONFIG_OPTION(
				    defaultForeground,
					"Specifies the index of the default foreground color in the palette.",
					"15",
				    unsigned
				);
				CONFIG_OPTION(
					defaultBackground,
					"Specifies the index of the default background color in the palette.",
					"0",
				    unsigned
				);
				/** Provides a value getter on the entire palette configuration group which returns the palette with the default colors set accordingly. 
				 */
				ui::AnsiTerminal::Palette operator () () const {
					ui::AnsiTerminal::Palette result{colors()};
					result.setDefaultForegroundIndex(defaultForeground());
					result.setDefaultBackgroundIndex(defaultBackground());
					return result;
				}
			);
			CONFIG_OPTION(
				confirmPaste,
				"Determines whether pasting into terminal should be explicitly confirmed. Allowed values are 'never', 'always', 'multiline'.",
				"\"multiline\"",
			    std::string
			);
			CONFIG_GROUP(
				cursor,
			    "Cursor properties",
                CONFIG_OPTION(
                    codepoint,
                    "UTF codepoint of the cursor",
                    "0x2581",
                    unsigned
                );
                // TODO change to color
                CONFIG_OPTION(
                    color,
                    "Color of the cursor",
                    "\"#ffffff\"",
                    ui::Color
                );
                CONFIG_OPTION(
                    blink,
                    "Determines whether the cursor blinks or not.",
                    "true",
                    bool
                );
				/** Value getter for cursor properies aggregated in a ui::Cursor object.
				 */
				ui::Cursor operator () () const {
					ui::Cursor result{};
					result.setVisible(true);
					result.setCodepoint(codepoint());
					result.setColor(color());
					result.setBlink(blink());
					return result;
				}
			);
			CONFIG_OPTION(
				inactiveCursorColor,
				"Color of the rectangle showing the cursor position when not focused.",
				"\"#00ff00\"",
			    ui::Color
			);
			CONFIG_GROUP(
				remoteFiles,
			    "Settings for opening remote files from the terminal locally.",
				CONFIG_OPTION(
					dir,
					"Directory to which the remote files should be downloaded. If empty, temporary directory will be used.",
					DefaultRemoteFilesDir,
				    std::string
				);
			);
			CONFIG_GROUP(
				sequences,
			    "Behavior customization for terminal escape sequences.",
				CONFIG_OPTION(
					boldIsBright,
					"If true, bold text is rendered in bright colors.",
					"true",
				    bool
				);
			);
            /*
			CONFIG_OPTION(
				log,
				"File to which all terminal input should be logged",
				"\"\"",
			    std::string
			);
            */
		);

		static Config & Instance() {
			static Config singleton{};
			return singleton;
		}

		/** Populates the configuration with the user specified configuration file and command line arguments. 
		 
		    First attempts to update the configuration with stored user settings. If the settings are not valid JSON, or if there are any other errors with the settings, they are backed up in a separate file first and then their cleaned version is saved. 

			If any settings for which values were not provided and default values can be calculated (i.e. by a function, not a JSON literal), these are calculated and the settings are updated and saved. 

			Finally, the command line arguments are parsed and the configuration updated accordingly (but not saved).  
		 */
		static Config & Setup(int argc, char * argv[]) {
			Config & result = Instance();
			result.setup(argc, argv);
			return result;
		}

		/** Returns the directory in which the configuration files should be located. 
		 */
	    static std::string GetSettingsFolder();

		/** Returns the actual settings file, i.e. the `settings.json` file in the settings directory. 
		 */
	    static std::string GetSettingsFile();

	protected:

	    Config() {
			initializationDone();
		}

		void setup(int argc, char * argv[]);

		/** Constructs the command line arguments parser connected to the configuration and parses the command line arguments. 
		 */
	    void parseCommandLine(int argc, char * argv[]) {
			// this is a shortcut if there are no arguments on the commandline. Note that this is conditional of none of the arguments being required, which is reasonable for the terminal's main application
			if (argc == 1)
			    return;
			JSONArguments args{};
#if (defined ARCH_WINDOWS)
			args.addArgument("Pty", {"--pty"}, session.pty);
#endif
			args.addArgument("FPS", {"--fps"}, renderer.fps);
			args.addArgument("Columns", {"--cols", "-c"}, session.cols);
			args.addArgument("Rows", {"--rows", "-r"}, session.cols);
			args.addArgument("Font Family", {"--font",}, font.family);
			args.addArgument("Font Size", {"--font-size"}, font.size);
			//args.addArgument("Log File", {"--log-file"}, session.log);
			args.addArrayArgument("Command", {"-e"}, session.command, /* isLast */ true);
			// once built, parse the command line
			args.parse(argc, argv);
		}

		void verifyConfigurationVersion(JSON & userConfig);

		/** \name Default value providers
		 
		    These static methods calculate default values for the complex configuration properties. The idea is that these will be executed once the terminal is installed, they will analyze the system and calculate proper values to be stored in the configuration file. 
		 */
		//@{

		/** Returns the version of the terminal++ binary. 
		 
		    This version is specified in the CMakeLists.txt and is watermarked to each binary.
		 */
	    static std::string TerminalVersion();

		static std::string DefaultTelemetryDir();

		static std::string DefaultRemoteFilesDir();		

		static std::string DefaultFontFamily();

		static std::string DefaultDoubleWidthFontFamily();

		static std::string DefaultSessionPTY();

		static std::string DefaultSessionCommand();

		//@}

#if (defined ARCH_WINDOWS)
        /** Determines whether the WSL is installed or not. 
         
            Returns the default WSL distribution, if any, or empty string if no WSL is present. 
         */
		static std::string IsWSLPresent();

        /** Determines if the ConPTY bypass is present in the WSL or not. 
         */
		static bool IsBypassPresent();

		static void UpdateWSLDistributionVersion(std::string & wslDistribution);

        /** Installs the bypass for given WSL distribution. 
         
            Returns true if the installation was successful, false otherwise. The bypass is installed by downloading the appropriate bypass binary from github releases and installing it into BYPASS_FOLDER (set in config.h).
         */
		static bool InstallBypass(std::string const & wslDistribution);
#endif

	};

    #endif // foobar

} // namespace tpp
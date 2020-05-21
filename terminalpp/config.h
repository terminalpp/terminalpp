#pragma once
#include "helpers/process.h"
#include "helpers/json_config.h"

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
#define SHORTCUT_PASTE (Key::V + Key::Ctrl + Key::Shift)

namespace helpers {

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

	/** Parts of the command argument are just JSON strings so they are only surrounded by double quotes. 
	 */
	template<>
	inline std::string JSONArguments::ConvertToJSON<Command>(std::string const & value) {
		return STR("\"" << value << "\"");
	}

} // namespace helpers

namespace tpp {	

    using helpers::JSONConfig;

	class Config : public JSONConfig::Root {
	public:
	    CONFIG_OPTION(
			version,
			"Version of tpp the settings are intended for, to make sure the settings are useful and to detect version changes",
			TerminalVersion,
			std::string			
		);

        /*
		CONFIG_GROUP(
			log,
		    "Log Properties", 
		    CONFIG_OPTION(
				dir,
				"Directory where to keep the log files",
				DefaultLogDir,
			    std::string
			);
		    CONFIG_OPTION(
				maxFiles, 
				"Maximum number of log files that tpp can keep at one time", 
				"100",
			    unsigned
			);
		);
        */

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
			    helpers::Command
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
			helpers::JSONArguments args{};
#if (defined ARCH_WINDOWS)
			args.addArgument("Pty", {"--pty"}, session.pty);
#endif
			args.addArgument("FPS", {"--fps"}, renderer.fps);
			args.addArgument("Columns", {"--cols", "-c"}, session.cols);
			args.addArgument("Rows", {"--rows", "-r"}, session.cols);
			args.addArgument("Font Family", {"--font",}, font.family);
			args.addArgument("Font Size", {"--font-size"}, font.size);
			//args.addArgument("Log File", {"--log-file"}, session.log);
			args.addArrayArgument("Command", {"-e"}, session.command, true);
			// once built, parse the command line
			args.parse(argc, argv);
		}

		void verifyConfigurationVersion(helpers::JSON & userConfig);

		/** \name Default value providers
		 
		    These static methods calculate default values for the complex configuration properties. The idea is that these will be executed once the terminal is installed, they will analyze the system and calculate proper values to be stored in the configuration file. 
		 */
		//@{

		/** Returns the version of the terminal++ binary. 
		 
		    This version is specified in the CMakeLists.txt and is watermarked to each binary.
		 */
	    static std::string TerminalVersion();

		static std::string DefaultLogDir();

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

} // namespace tpp
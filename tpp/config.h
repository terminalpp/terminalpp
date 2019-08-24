#pragma once
#include "helpers/args.h"
#include "helpers/process.h"
#include "helpers/json.h"

#define SHOW_LINE_ENDINGS


#define DEFAULT_WINDOW_TITLE "t++"

/** Determines the default blink speed of the cursor or blinking text. 
 */ 
#define DEFAULT_BLINK_SPEED 500


/** Keyboard shortcuts for various actions.
 */
#define SHORTCUT_ZOOM_IN (ui::Key::Equals + ui::Key::Ctrl)
#define SHORTCUT_ZOOM_OUT (ui::Key::Minus + ui::Key::Ctrl)
#define SHORTCUT_FULLSCREEN (ui::Key::Enter + ui::Key::Alt)
#define SHORTCUT_PASTE (ui::Key::V + ui::Key::Ctrl + ui::Key::Shift)


namespace tpp {	
	
	class Config {
	public:

		unsigned rendererFps() const {
			return json_["renderer"]["fps"].value<int>();
		}

		std::string const & fontFamily() const {
			return json_["font"]["family"].value<std::string>();
		}

		unsigned fontSize() const {
			return json_["font"]["size"].value<int>();
		}

		std::string const & sessionPTY() const {
			return json_["session"]["pty"].value<std::string>();
		}

		helpers::Command sessionCommand() const {
			NOT_IMPLEMENTED;
		}

		unsigned sessionCols() const {
			return json_["session"]["cols"].value<int>();
		}

		unsigned sessionRows() const {
			return json_["session"]["rows"].value<int>();
		}

		bool sessionFullscreen() const {
			return json_["session"]["fullscreen"].value<bool>();
		}
		
	    
		/** Returns the singleton instance of the configuration. 
		 */
		static Config const & Instance() {
			Config * config = Singleton_();
			ASSERT(config != nullptr) << "Configuration not initialized";
			return * config;
		}

		/** Initializes the configuration and returns the pointer to the config singleton. 
		 */
	    static Config const & Initialize(int argc, char * argv[]);

	private:

		/** Internally, the configuration is held in a JSON. 
		 */
	    helpers::JSON json_;

		/** Private default constructor does nothing since config should always be constructed with the parsed JSON object. 
		 */
	    Config(helpers::JSON && json):
		    json_(std::move(json)) {
		}

		void processCommandLineArguments(int argc, char * argv[]);

	    static Config * & Singleton_() {
			static Config * singleton = nullptr;
			return singleton;
		}

		static helpers::JSON GetJSONSettings();

		static helpers::JSON CreateDefaultJSONSettings();


	};
	
	namespace config {



		/** Maximum number of frames per second the renderer is allowed to render.

			There is very little point in setting this higher than the screen's v-sync rate. 30 fps seems to be just fine, 60 fps means more updated and greater stress on the terminal.
		 */
		extern helpers::Arg<unsigned> FPS;
        #define DEFAULT_TERMINAL_FPS 60

		/** Initial size of the terminal window and the underlying terminal in text columns an rows.
		 */
		//extern helpers::Arg<unsigned> Cols;
		//extern helpers::Arg<unsigned> Rows;
		//#define DEFAULT_TERMINAL_COLS 80
		//#define DEFAULT_TERMINAL_ROWS 25

		/** Name of the font to be used and its size in pixels for zoom 1.
		 */
		//extern helpers::Arg<std::string> FontFamily;
		//extern helpers::Arg<unsigned> FontSize;
		//#define DEFAULT_TERMINAL_FONT_FAMILY "Iosevka Term"
		//#define DEFAULT_TERMINAL_FONT_SIZE 18

		/** The command to be executed.

			This is usually the shell. Depends on the OS used and pseudoterminal interface.

			This particular command starts the bypass in wsl, overrides the SHELL envvar to bash (default) and then instructs the bypass to run bash.
		 */
		extern helpers::Arg<std::vector<std::string>> Command;
#ifdef ARCH_WINDOWS
		#define DEFAULT_TERMINAL_COMMAND "wsl", "-e", "/home/peta/devel/tpp-build/bypass/bypass", "SHELL=/bin/bash", "-e", "bash"
		#define DEFAULT_CONPTY_COMMAND "wsl"
#else
        #define DEFAULT_TERMINAL_COMMAND "bash"
#endif

#ifdef ARCH_WINDOWS
		/** Specifies the pseudoterminal interface to be used.

			The default pseudoterminal on the given platform is `vterm::LocalPTY`. However on windows this terminal performs its own destructive operations on the I/O. A `BypassPTY` was created, which in a simple program running in the wsl which translates the terminal communication and events over simple process I/O, thus bypassing the system pseudoterminal.
		 */
		//extern helpers::Arg<bool> UseConPTY;
#endif

} } // namespace tpp::config
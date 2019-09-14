#pragma once
#include "helpers/args.h"
#include "helpers/process.h"
#include "helpers/json.h"

//#define SHOW_LINE_ENDINGS


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

		std::string const & version() const {
			return get({"version"});
		}

		std::string const & logFile() const {
			return get({"log", "file"});
		}

		unsigned rendererFps() const {
			return get({"renderer", "fps"});
		}

		std::string const & fontFamily() const {
			return get({"font", "family"});
		}

		std::string const & doubleWidthFontFamily() const {
			return get({"font", "doubleWidthFamily"});
		}

		unsigned fontSize() const {
			return get({"font", "size"});
		}

		std::string const & sessionPTY() const {
			return get({"session", "pty"});
		}

		helpers::Command sessionCommand() const {
			std::vector<std::string> cmd;
			for (helpers::JSON const & x : get({"session", "command"}))
				cmd.push_back(static_cast<std::string const &>(x));
			return helpers::Command(cmd);
		}

		unsigned sessionCols() const {
			return get({"session", "cols"});
		}

		unsigned sessionRows() const {
			return get({"session", "rows"});
		}

		unsigned sessionFullscreen() const {
			return get({"session", "fullScreen"});
		}
		
		helpers::JSON & json() {
			return json_;
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

		/** Returns the JSON element with given path from the configuration root, or nullptr if the path is invalid. 
		 */
		helpers::JSON const & get(std::initializer_list<char const *> path) const {
			helpers::JSON const * result = & json_;
			for (char const *c : path)
				result = &(*result)[c];
			return *result;
		}

		void processCommandLineArguments(int argc, char * argv[]);

		void saveSettings();

		/** Converts the settings file if the version does not correspond to the expected version. 
		 */
		void updateToNewVersion();

		void copyMissingSettingsFrom(helpers::JSON & settings, helpers::JSON & defaults);

	    static Config * & Singleton_() {
			static Config * singleton = nullptr;
			return singleton;
		}

		static helpers::JSON ReadSettings();

		static helpers::JSON CreateDefaultSettings();

	}; // tpp::Config

} // namespace tpp
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
			std::vector<std::string> cmd;
			for (helpers::JSON const & x : json_["session"]["command"])
			    cmd.push_back(x.value<std::string>());
			return helpers::Command(cmd);
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

		void processCommandLineArguments(int argc, char * argv[]);

	    static Config * & Singleton_() {
			static Config * singleton = nullptr;
			return singleton;
		}

		static helpers::JSON GetJSONSettings();

		static helpers::JSON CreateDefaultJSONSettings();


	}; // tpp::Config

} // namespace tpp
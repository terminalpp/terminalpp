#include "settings_json.h"

#include "config.h"

namespace tpp { 
	
	Config const & Config::Initialize(int argc, char * argv[]) {
		Config * & config = Singleton_();
		ASSERT(config == nullptr) << "Already initialized";
		config = new Config(GetJSONSettings());
		//config->processCommandLineArguments(argc, argv);


		return * config;
	}

	void Config::processCommandLineArguments(int argc, char * argv[]) {
#ifdef ARCH_WINDOWS
		helpers::Arg<bool> UseConPTY(
			{ "--use-conpty" },
			sessionPTY() == "local",
			false,
			"Uses the Win32 ConPTY pseudoterminal instead of the WSL bypass"
		);
#endif
		helpers::Arg<unsigned> FPS(
			{ "--fps" },
			rendererFps(),
			false,
			"Maximum number of fps the terminal will display"
		);
		helpers::Arg<unsigned> Cols(
			{ "--cols", "-c" },
			sessionCols(),
			false,
			"Number of columns of the terminal window"
		);
		helpers::Arg<unsigned> Rows(
			{ "--rows", "-r" },
			sessionRows(),
			false,
			"Number of rows of the terminal window"
		);
		helpers::Arg<std::string> FontFamily(
			{ "--font" }, 
			fontFamily(), 
			false, 
			"Font to render the terminal with"
		);
		helpers::Arg<unsigned> FontSize(
			{ "--font-size" }, 
			fontSize(),
			false,
			"Size of the font in pixels at no zoom."
		);
		helpers::Arg<std::vector<std::string>> Command(
			{ "-e" }, 
			{ DEFAULT_TERMINAL_COMMAND },
			false,
			"Determines the command to be executed in the terminal",
			true
		);



	}

	helpers::JSON Config::GetJSONSettings() {
		// TODO this should be updated and we should check platform dependent if the settings are set
		return CreateDefaultJSONSettings();
	}

	helpers::JSON Config::CreateDefaultJSONSettings() {
		helpers::JSON json(helpers::JSON::Parse(DefaultJSONSettings()));
		// TODO platform dependent stuff here
		return json;
	}



	
	namespace config {
/*
#ifdef ARCH_WINDOWS
		helpers::Arg<bool> UseConPTY(
			{ "--use-conpty" },
			false,
			false,
			"Uses the Win32 ConPTY pseudoterminal instead of the WSL bypass"
		);
#endif
*/
		helpers::Arg<unsigned> FPS(
			{ "--fps" },
			DEFAULT_TERMINAL_FPS,
			false,
			"Maximum number of fps the terminal will display"
		);

/*		
		helpers::Arg<unsigned> Cols(
			{ "--cols", "-c" },
			DEFAULT_TERMINAL_COLS,
			false,
			"Number of columns of the terminal window"
		);
		helpers::Arg<unsigned> Rows(
			{ "--rows", "-r" },
			DEFAULT_TERMINAL_ROWS,
			false,
			"Number of rows of the terminal window"
		);
		helpers::Arg<std::string> FontFamily(
			{ "--font" }, 
			DEFAULT_TERMINAL_FONT_FAMILY, 
			false, 
			"Font to render the terminal with"
		);
		helpers::Arg<unsigned> FontSize(
			{ "--font-size" }, 
			DEFAULT_TERMINAL_FONT_SIZE,
			false,
			"Size of the font in pixels at no zoom."
		);

		*/
		helpers::Arg<std::vector<std::string>> Command(
			{ "-e" }, 
			{ DEFAULT_TERMINAL_COMMAND },
			false,
			"Determines the command to be executed in the terminal",
			true
		);


}} // namespace tpp::config
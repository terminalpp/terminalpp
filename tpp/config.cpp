#include <fstream>

#include "settings_json.h"

#include "stamp.h"
#include "helpers/stamp.h"

#include "application.h"
#include "config.h"

namespace tpp { 
	
	Config const & Config::Initialize(int argc, char * argv[]) {
		Config * & config = Singleton_();
		ASSERT(config == nullptr) << "Already initialized";
		config = new Config(GetJSONSettings());
		config->processCommandLineArguments(argc, argv);

		return * config;
	}

	void Config::processCommandLineArguments(int argc, char * argv[]) {
		// initialize the arguments
#if (defined ARCH_WINDOWS)
		helpers::Arg<bool> useConPTY(
			{ "--use-conpty" },
			sessionPTY() == "local",
			false,
			"Uses the Win32 ConPTY pseudoterminal instead of the WSL bypass"
		);
#endif
		helpers::Arg<unsigned> fps(
			{ "--fps" },
			rendererFps(),
			false,
			"Maximum number of fps the terminal will display"
		);
		helpers::Arg<unsigned> cols(
			{ "--cols", "-c" },
			sessionCols(),
			false,
			"Number of columns of the terminal window"
		);
		helpers::Arg<unsigned> rows(
			{ "--rows", "-r" },
			sessionRows(),
			false,
			"Number of rows of the terminal window"
		);
		helpers::Arg<std::string> fontFamily(
			{ "--font" }, 
			this->fontFamily(), 
			false, 
			"Font to render the terminal with"
		);
		helpers::Arg<unsigned> fontSize(
			{ "--font-size" }, 
			this->fontSize(),
			false,
			"Size of the font in pixels at no zoom."
		);
		helpers::Arg<std::vector<std::string>> command(
			{ "-e" }, 
			{ },
			false,
			"Determines the command to be executed in the terminal",
			true
		);
		// process the arguments
		helpers::Arguments::SetVersion(STR("t++ :" << helpers::Stamp::Stored()));		
		helpers::Arguments::Parse(argc, argv);
		// now update any settings according to the specified arguments
#if (defined ARCH_WINDOWS)
		if (useConPTY.specified())
		    json_["session"]["pty"] = (*useConPTY ? "local" : "bypass");
#endif
        if (fps.specified())
		    json_["renderer"]["fps"] = static_cast<int>(*fps);
        if (cols.specified())
		    json_["session"]["cols"] = static_cast<int>(*cols);
        if (rows.specified())
		    json_["session"]["rows"] = static_cast<int>(*rows);
        if (fontFamily.specified())
		    json_["font"]["family"] = *fontFamily;
        if (fontSize.specified())
		    json_["font"]["size"] = static_cast<int>(*fontSize);
		if (command.specified()) {
			helpers::JSON & cmd = json_["session"]["command"];
			cmd.clear();
			for (std::string const & s : *command)
			    cmd.add(helpers::JSON(s));
		}
	}

	helpers::JSON Config::GetJSONSettings() {
		std::string settingsFile(Application::Instance()->getSettingsFolder() + "settings.json");
		std::ifstream sf(settingsFile);
		if (sf.good()) {
		    return helpers::JSON::Parse(sf);
		} else  {
		    helpers::JSON json(CreateDefaultJSONSettings());
			// TODO enable this when ready and platform defaults are working properly
			//std::ofstream sfo(settingsFile);
			//sfo << json;
			return json;
		}
	}

	helpers::JSON Config::CreateDefaultJSONSettings() {
		helpers::JSON json(helpers::JSON::Parse(DefaultJSONSettings()));
		Application::Instance()->updateDefaultSettings(json);
		return json;
	}



	

} // namespace tpp
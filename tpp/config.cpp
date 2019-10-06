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
		helpers::JSON json = ReadSettings();
		if (json.isNull()) {
			Application::Instance()->alert("No settings found, initializing from defaults");
		    json = CreateDefaultSettings();
		    config = new Config(std::move(json));
			Application::Instance()->updateDefaultSettings(config->json_);
			config->saveSettings();
		} else {
		    config = new Config(std::move(json));
			if (config->version() != JSONSettingsVersion()) {
				Application::Instance()->alert("Settings will be updated to new version. Existing values will be preserved where possible");
			    config->updateToNewVersion();
				Application::Instance()->updateDefaultSettings(config->json_);
                config->saveSettings();
			}
		}

		config->processCommandLineArguments(argc, argv);

		return * config;
	}

	void Config::OpenSettingsInEditor() {
		Application::Open(GetSettingsLocation(), /* edit = */ true);
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
		helpers::Arg<std::string> logFile(
			{ "--log-file" }, 
			"", 
			false, 
			"File to which all terminal input will be logged, if specified"
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
		if (logFile.specified())
		    json_["log"]["file"] = *logFile;
		else
		    json_["log"]["file"] = std::string{};
	}

	void Config::saveSettings() {
		std::string settingsFile{GetSettingsLocation()};
		std::ofstream sf(settingsFile);
		sf << json_;
	}

	void Config::updateToNewVersion() {
		// parse the default settings and update them to any runtime calculated values
		helpers::JSON defaults = CreateDefaultSettings();
		// update the version
		json_["version"] = JSONSettingsVersion();
		// now check the defaults
		copyMissingSettingsFrom(json_, defaults);
	}

	void Config::copyMissingSettingsFrom(helpers::JSON & settings, helpers::JSON & defaults) {
		// make sure that we are dealing with objects only
		ASSERT(settings.kind() == helpers::JSON::Kind::Object && defaults.kind() == helpers::JSON::Kind::Object);
		// copy all missing setings
		for (helpers::JSON::Iterator i = defaults.begin(), e = defaults.end(); i != e; ++i) {
			if (!settings.hasKey(i.name()) || settings[i.name()].kind() != i->kind()) { 
				settings[i.name()] = std::move(*i);
			} else {
				// if the settings element is an object itself, recurse 
				if (i->kind() == helpers::JSON::Kind::Object)
				    copyMissingSettingsFrom(settings[i.name()], *i);
			}
		}
	}

	std::string Config::GetSettingsLocation() {
		return Application::Instance()->getSettingsFolder() + "settings.json";
	}

	helpers::JSON Config::ReadSettings() {
		std::string settingsFile{GetSettingsLocation()};
		std::ifstream sf(settingsFile);
		if (sf.good()) {
			helpers::JSON result(helpers::JSON::Parse(sf));
			// backwards compatibility with version 0.2 where version was double
			// TODO to be deleted later as a dead code
			if (result["version"].kind() == helpers::JSON::Kind::Double)
			    result["version"] = STR(static_cast<double>(result["version"]));
		    return result;
		} else {
			return helpers::JSON(nullptr);
		}
	}

	helpers::JSON Config::CreateDefaultSettings() {
		helpers::JSON json(helpers::JSON::Parse(DefaultJSONSettings()));
		json["version"] = JSONSettingsVersion();
		return json;
	}

} // namespace tpp
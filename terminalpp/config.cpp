#include <fstream>
#include <filesystem>

#include "helpers/stamp.h"

#include "application.h"
#include "config.h"



namespace tpp { 

	std::string Config::GetSettingsFolder() {
        return helpers::JoinPath(helpers::LocalSettingsFolder(), "terminalpp");
	}

	std::string Config::GetSettingsFile() {
		return helpers::JoinPath(GetSettingsFolder(), "settings.json");
	}

	void Config::setup(int argc, char * argv[]) {
		bool saveSettings = false;
		std::string filename{GetSettingsFile()};
		// load user settings first
		{
			std::ifstream f{filename};
			if (f.good()) {
				try {
					helpers::JSON settings{helpers::JSON::Parse(f)};
					verifyConfigurationVersion(settings);
					// TODO specify, check errors, make copy if wrong, etc. 
					specify(settings, [& saveSettings](std::exception const & e){
						Application::Alert(e.what());
						saveSettings = true;
					});
				} catch (std::exception const & e) {
					Application::Alert(e.what());
					saveSettings = true;
				}
				// if there were any errors with the settings, create backup of the old settings as new settings will be stored. 
				if (saveSettings) {
					std::string backup{helpers::MakeUnique(filename)};
					Application::Alert(STR("New settings file will be saved, backup stored in " << backup));
					f.close();
					helpers::Rename(filename, backup);
				}
			// if there are no settings, then the settings should be saved after default values are calculated
			} else {
				saveSettings = true;
				Application::Alert(STR("No settings file found, default settings will be calculated and stored in " << filename));
			}
		}
		saveSettings = fixMissingDefaultValues() || saveSettings;
		// if the settings should be saved, save them now 
		if (saveSettings) {
            helpers::CreatePath(GetSettingsFolder());
			std::ofstream f(filename);
			if (!f)
			    THROW(helpers::IOError()) << "Unable to write to the settings file " << filename;
			// get the saved JSON and store it in the settings file
			f << save();
		}
		// read the command line arguments and update the configuration accordingly
		parseCommandLine(argc, argv);
	}

	void Config::verifyConfigurationVersion(helpers::JSON & userConfig) {
		if (userConfig.hasKey("version")) {
			if (userConfig["version"].toString() == PROJECT_VERSION)
			    return;
			userConfig.erase("version");
		}
		Application::Alert(STR("Settings version differs from current terminal version (" << PROJECT_VERSION << "). The configuration will be updated to the new version."));
	}


	// default configfuration providers

	std::string Config::TerminalVersion() {
		return helpers::Quote(PROJECT_VERSION);
	}

	std::string Config::DefaultLogDir() {
		return helpers::Quote(helpers::JoinPath(helpers::JoinPath(helpers::TempDir(), "terminalpp"),"logs"));
	}

	std::string Config::DefaultRemoteFilesDir() {
		return helpers::Quote(helpers::JoinPath(helpers::JoinPath(helpers::TempDir(), "terminalpp"),"remoteFiles"));
	}

	std::string Config::DefaultFontFamily() {
#if (defined ARCH_WINDOWS)
		return "\"Consolas\"";
#elif (defined ARCH_MACOS)
        return "\"Courier New\"";
#else
		char const * fonts[] = { "Monospace", "DejaVu Sans Mono", "Nimbus Mono", "Liberation Mono", nullptr };
		char const ** f = fonts;
		while (*f != nullptr) {
			std::string found{helpers::Exec(helpers::Command("fc-list", { *f }), "")};
			if (! found.empty())
			    return STR("\"" << *f << "\"");
			++f;
		}
		Application::Alert("Cannot guess valid font - please specify manually for best results");
		return "\"\"";
#endif
	}

	std::string Config::DefaultDoubleWidthFontFamily() {
		return DefaultFontFamily();
	}

	std::string Config::DefaultSessionPTY() {
#if (defined ARCH_WINDOWS)
        auto & cmd = Config::Instance().session.command;
		if (cmd.specified()) 
		    THROW(helpers::ArgumentError()) << "Command is specified, but PTY is not, which can lead to inconsistent result. Please specify or clear both manually in the configuration file";
        std::string wslDefaultDistro{IsWSLPresent()};
		if (! wslDefaultDistro.empty()) {
			bool hasBypass = IsBypassPresent();
			if (!hasBypass) {
				if (MessageBox(nullptr, L"WSL bypass was not found in your default distribution. Do you want terminal++ to install it? (if No, ConPTY will be used instead)", L"WSL Bypass not found", MB_ICONQUESTION + MB_YESNO) == IDYES) {
					// update the version of the distro, if necessary
					UpdateWSLDistributionVersion(wslDefaultDistro);
					// attempt to install the bypass
					hasBypass = InstallBypass(wslDefaultDistro);
					if (hasBypass)
						MessageBox(nullptr, L"WSL Bypass successfully installed", L"Success", MB_ICONINFORMATION + MB_OK);
					else
						MessageBox(nullptr, L"Bypass installation failed, most likely due to missing binary for your WSL distribution. Terminal++ will continue with ConPTY.", L"WSL Install bypass failure", MB_ICONSTOP + MB_OK);
				}
			}
			if (hasBypass)
			    return "\"bypass\"";
		}
		return "\"local\"";
#else
		return "\"local\"";
#endif
	}

	std::string Config::DefaultSessionCommand() {
#if (defined ARCH_WINDOWS)
        auto & pty = Config::Instance().session.pty;
		// make sure we know which PTY to use before we attempt to determine the command
		if (! pty.specified())
		    pty.set(DefaultSessionPTY());
		if (pty() == "bypass")
		    return STR("[\"wsl.exe\", \"--\", \"" << BYPASS_PATH << "\"]");
		else 
		    return "[\"wsl.exe\"]"; 
#elif (defined ARCH_MACOS)
		// get the default shell for the current user with the --login flag so that the .profile is loaded as well
		return STR("[\"" << getpwuid(getuid())->pw_shell << "\", \"--login\"]");
#else
		// get the default shell for the current user
		return STR("[\"" << getpwuid(getuid())->pw_shell << "\"]");
#endif
	}

#if (defined ARCH_WINDOWS)
	std::string Config::IsWSLPresent() {
		helpers::ExitCode ec;
		std::vector<std::string> lines = helpers::SplitAndTrim(helpers::Exec(helpers::Command("wsl.exe", {"-l"}),"", &ec), "\n");
	    if (lines.size() > 1 && lines[0] == "Windows Subsystem for Linux Distributions:") {
			for (size_t i = 1, e = lines.size(); i != e; ++i) {
				if (helpers::EndsWith(lines[i], "(Default)"))
				    return helpers::Split(lines[i], " ")[0];
			}
		}
		// nothing found, return empty string
		return std::string{};
	}

	bool Config::IsBypassPresent() {
		helpers::ExitCode ec;
		std::string output{helpers::Exec(helpers::Command("wsl.exe", {"--", BYPASS_PATH, "--version"}), "", &ec)};
		return output.find("Terminal++ Bypass, version") == 0;
	}

	void Config::UpdateWSLDistributionVersion(std::string & wslDistribution) {
		if (wslDistribution == "Ubuntu") {
    		helpers::ExitCode ec;
			std::vector<std::string> lines = helpers::SplitAndTrim(
				helpers::Exec(helpers::Command("wsl.exe", {"--", "lsb_release", "-a"}), "", &ec),
				"\n"
			);
			for (std::string const & line : lines) {
				if (helpers::StartsWith(line, "Release:")) {
					std::string ver = helpers::Trim(line.substr(9));
					wslDistribution = wslDistribution + "-" + ver;
					break;
				}
			}
		}
	}

	bool Config::InstallBypass(std::string const & wslDistribution) {
		try {
			std::string url{STR("https://github.com/terminalpp/bypass/releases/download/v0.1/tpp-bypass-" << wslDistribution)};
			helpers::Exec(helpers::Command("wsl.exe", {"--", "mkdir", "-p", BYPASS_FOLDER}), "");
			helpers::Exec(helpers::Command("wsl.exe", {"--", "wget", "-O", BYPASS_PATH, url}), "");
			helpers::Exec(helpers::Command("wsl.exe", {"--", "chmod", "+x", BYPASS_PATH}), "");
		} catch (...) {
			return false;
		}
		return IsBypassPresent();
	}
#endif

} // namespace tpp
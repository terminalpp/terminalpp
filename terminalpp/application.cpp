#include "config.h"
#include "application.h"


namespace tpp {

    std::string Application::checkLatestVersion(std::string const & channel) {
        try {
            JSON versions{getLatestVersion()};
            // if the required channel is not present, the version check is unsuccessful
            if (! versions.hasKey(channel))
                return std::string{};
            return versions[channel];
        } catch (...) {
            return std::string{};
        }
    }

    JSON Application::getLatestVersion() {
        return JSON::Parse(Curl("https://terminalpp.com/versions.json"));
    }

    JSON Application::getCachedLatestVersion() {
        std::ifstream f{Config::GetSettingsFolder() + "/versions.json"};
        if (f.good())
            return JSON::Parse(f);
        else
            return JSON{JSON::Kind::Object};
    }

} // namespace tpp
#include "config.h"
#include "application.h"


namespace tpp {

    std::string Application::checkLatestVersion(std::string const & channel) {
        try {
            helpers::JSON versions{getLatestVersion()};
            // if the required channel is not present, the version check is unsuccessful
            if (! versions.hasKey(channel))
                return std::string{};
            return versions[channel];
        } catch (...) {
            return std::string{};
        }
    }

    helpers::JSON Application::getLatestVersion() {
        return helpers::JSON::Parse(helpers::Curl("https://terminalpp.com/versions.json"));
    }

    helpers::JSON Application::getCachedLatestVersion() {
        std::ifstream f{Config::GetSettingsFolder() + "/versions.json"};
        if (f.good())
            return helpers::JSON::Parse(f);
        else
            return helpers::JSON{helpers::JSON::Kind::Object};
    }

} // namespace tpp
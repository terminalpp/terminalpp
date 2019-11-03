#include "helpers/filesystem.h"
#include "application.h"

namespace tpp {

    void Application::updateDefaultSettings(helpers::JSON & json) {
        // if no log has been specified, create one in the tmp folder
        if (json["log"]["dir"].empty()) 
            json["log"]["dir"] = helpers::JoinPath(helpers::JoinPath(helpers::TempDir(), "terminalpp"),"logs");
        // if no directory has been set for the remote files, create one in the tmp folder
        if (json["session"]["remoteFiles"]["dir"].empty()) 
            json["session"]["remoteFiles"]["dir"] = helpers::JoinPath(helpers::JoinPath(helpers::TempDir(), "terminalpp"),"remoteFiles");
    };


} // namespace tpp
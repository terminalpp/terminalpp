#pragma once

#include "helpers/helpers.h"
#include "helpers/json.h"
#include "helpers/curl.h"

#include "stamp.h"

#include "ui/event_queue.h"

namespace tpp {

    class Window;

    class Application {
    public:

        static std::string Stamp() {
            std::stringstream result;
            if (!stamp::version.empty())
                result << "    version:    " << stamp::version << std::endl;
            result << "    commit:     " << stamp::commit << (stamp::dirty ? "*" : "") << std::endl;
            #if (defined RENDERER_QT)
                result << "    platform:   " << ARCH << "(Qt) " << ARCH_SIZE << " " << ARCH_COMPILER << " " << ARCH_COMPILER_VERSION << " " << stamp::build << std::endl;
            #else
                result << "    platform:  " << ARCH << "(native) " << ARCH_SIZE << " " << ARCH_COMPILER << " " << ARCH_COMPILER_VERSION << " " << stamp::build << std::endl;
            #endif
            result << "    build time: " << stamp::build_time << std::endl;
            return result.str();
        }

        static Application * Instance() {
            return Singleton_();
        }

        virtual ~Application() {
        }

        /** Displays an alert box with single button to dismiss. 
         */
        virtual void alert(std::string const & message) = 0;

        virtual bool query(std::string const & title, std::string const & message) = 0;

        /** Opens given local filename using the system viewer or editor. 
         
            Application implementations must provide native implementation of this functionality. 
         */
        virtual void openLocalFile(std::string const & filename, bool edit) = 0;

        /** Opens url in user's default browser. 
         */
        virtual void openUrl(std::string const & url) = 0;

        void createNewIssue(std::string const & title, std::string const & body) {
            std::string url{STR(
                "https://github.com/terminalpp/terminalpp/issues/new?title=" <<
                UrlEncode(title) <<
                "&body=" << UrlEncode(body) << "%0a%0a%3e" << UrlEncode(Stamp())
            )};
            openUrl(url);
        }

        virtual void setClipboard(std::string const & contents) = 0;

        /** Creates new renderer window. 
         */
        virtual Window * createWindow(std::string const & title, int cols, int rows) = 0;

        /** The main event loop of the application. 
         */
        virtual void mainLoop() = 0;

        /** Determines the latest versions available for specified channels. 
         
            Downloads the `https://terminalpp.com/versions.json` file which contains a list of latest versions available in various channels.
         */
        std::string checkLatestVersion(std::string const & channel);

    protected:
        Application() {
            ASSERT(Singleton_() == nullptr) << "Application assumed to be singleton";
            Singleton_() = this;
        }
        
        JSON getLatestVersion();
        JSON getCachedLatestVersion();

        ui::EventQueue eventQueue_;

    private:

        static Application *& Singleton_() {
            static Application * singleton_ = nullptr;
            return singleton_;
        }

    }; // tpp::Application

} // namespace tpp

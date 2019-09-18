#pragma once

#include "helpers/helpers.h"
#include "helpers/json.h"

namespace tpp {

    class Window;

    class Application {
    public:
        static Application * Instance() {
            return Singleton();
        }

        static void Alert(std::string const & message) {
            Instance()->alert(message);
        }

        virtual ~Application() {
        }

        /** Returns the folder to which the terminal should store its settings. 
         */
        virtual std::string getSettingsFolder() = 0;

        virtual Window * createWindow(std::string const & title, int cols, int rows, unsigned cellHeightPx) = 0;

        /** The main event loop of the application. 
         */
        virtual void mainLoop() = 0;

    protected:

        friend class Config;

        Application() {
            ASSERT(Singleton() == nullptr) << "Application assumed to be singleton";
            Singleton() = this;
        }

        /** Displays an alert box with single button to dismiss. 
         */
        virtual void alert(std::string const & message) = 0;


        virtual void updateDefaultSettings(helpers::JSON & json) = 0;

    private:

        static Application *& Singleton() {
            static Application * singleton_ = nullptr;
            return singleton_;
        }


    }; // tpp::Application


} // namespace tpp
#pragma once

#include "helpers/helpers.h"

namespace tpp {

    class Window;

    class Application {
    public:
        static Application * Instance() {
            return Singleton();
        }

        virtual ~Application() {
        }

        virtual Window * createWindow(std::string const & title, int cols, int rows, unsigned cellHeightPx) = 0;

        /** The main event loop of the application. 
         */
        virtual void mainLoop() = 0;

    protected:

        Application() {
            ASSERT(Singleton() == nullptr) << "Application assumed to be singleton";
            Singleton() = this;
        }

    private:

        static Application *& Singleton() {
            static Application * singleton_ = nullptr;
            return singleton_;
        }


    }; // tpp::Application


} // namespace tpp
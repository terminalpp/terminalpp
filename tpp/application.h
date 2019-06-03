#pragma once 

#include <unordered_set>

#include "terminal_window.h"

namespace tpp {

	class Session;

	class Application {
	public:

		/** Returns the application singleton. 
		 */
		static Application * Instance() {
			return instance_;
		}

		static void MainLoop() {
			instance_->mainLoop();
		}

		virtual ~Application() {

		}

		virtual TerminalWindow* createTerminalWindow(Session * session, TerminalWindow::Properties const& properties, std::string const & name) = 0;

		TerminalWindow::Properties const& defaultTerminalWindowProperties() const {
			return defaultTerminalWindowProperties_;
		}

	protected:

		Application() {
			ASSERT(instance_ == nullptr) << "Application must be a singleton";
			instance_ = this;
		}

		virtual void mainLoop() = 0;

		TerminalWindow::Properties defaultTerminalWindowProperties_;

		static Application* instance_;
	}; // tpp::Application

} // namespace tpp

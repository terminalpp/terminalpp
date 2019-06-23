#pragma once 

#include <unordered_set>

#include "terminal_window.h"

namespace tpp {

	class Session;

	class Application {
	public:

		/** Returns the application singleton. 
		 */
		template<typename T = Application>
		static T * Instance() {
			static_assert(std::is_base_of<Application, T>::value, "Must be child of Application");
#ifdef NDEBUG
			return reinterpret_cast<T*>(instance_);
#else
			return dynamic_cast<T*>(instance_);
#endif
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

		/** Returns the selection color. 
		 */
		vterm::Color selectionBackgroundColor() {
			return vterm::Color(0xc0, 0xc0, 0xff);
		}

	protected:

		virtual void mainLoop() = 0;

		virtual void sendBlinkTimerMessage() = 0;

		Application();

		TerminalWindow::Properties defaultTerminalWindowProperties_;

		static Application* instance_;
	}; // tpp::Application

} // namespace tpp

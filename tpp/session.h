#pragma once

#include <thread>
#include <mutex>
#include <condition_variable>
#include <unordered_set>


#include "helpers/log.h"
#include "helpers/process.h"

#include "vterm/terminal.h"
#include "vterm/local_pty.h"

#include "terminal_window.h"

namespace tpp {


	/** Encapsulates a single session in the terminal. 

	 */
	class Session : public helpers::Object {
	public:

		static Session* Create(std::string const& name, helpers::Command const& command) {
			return new Session(name, command);
		}

		static void Close(Session* session) {
			if (!session->closing_) {
                LOG << "Closing session...";
				session->closing_ = true;
				Sessions_.erase(session);
				delete session;
			}
		}

		/** Name of the session. 
		
		    Identifies the session. Used as the session's window title unless one provided by the application. 
		 */
		std::string const& name() const {
			return name_;
		}

		void setName(std::string const& name) {
			if (name_ != name) {
				name_ = name;
				// TODO notify terminal windows
			}
		}

		/** The command associated with the session. 

		    This is the command to be displayed in the terminal. Most likely a shell name, or a script to connect to remote machine. 

			TODO: Once more connection types are available, the command should change. 
		 */
		helpers::Command const& command() const {
			return command_;
		}

		void setCommand(helpers::Command const& command) {
			command_ = command;
			// TODO update console? 
		}

		/** Starts the session. 

		    
		 */
		void start();


		void show() {
			window_->show();
		}

	protected:

		/** Creates new session.
		 */
		Session(std::string const& name, helpers::Command const& command);

		/** Terminates the session.
		 */
		~Session();



	private:
		friend class TerminalWindow;


		/** Function called when the PTY attached to the session is terminated.
         */
		void onPTYTerminated(vterm::PTY::TerminatedEvent & e) {
			LOG << "PTY terminated " << *e;
			window_->close();
		}


		bool closing_;

		/** Name of the session. */
		std::string name_;

		/** Command to be executed for the terminal.
		 */
		helpers::Command command_;

		/** The PTY for the session. 
		 */
		vterm::PTY* pty_;

		/** The VT100 terminal backend. 
		 */
		vterm::Terminal * terminal_;

		/** Window associated with the terminal. 
		 */
		TerminalWindow * window_;

		/** Properties of the attached window. 
		 */
		TerminalWindow::Properties windowProperties_;

		static std::unordered_set<Session*> Sessions_;

	};



} // namespace tpp
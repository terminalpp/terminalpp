#pragma once



#include "vterm/terminal.h"

namespace tpp {

	/** Encapsulates a single session in the terminal. 

	    TODO: This should be the main hub for managing and persisting the session properties in tpp. 

	 */
	class Session {
	public:

		/** Creates new session. 
		 */
		Session(std::string const& name) :
			name_(name) {
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
		helpers::Commandconst& command() const {
			return command_;
		}

		void setCommand(helpers::Commandconst& cmd) {
			command_ = command;
			// TODO update console? 
		}

		/** Starts the session. 

		    
		 */
		void start() {
			ASSERT(terminal_ == nullptr) << "Session " << name_ << " already started";
			// create the terminal
			terminal_ = new vterm::Terminal(windowProperties_.cols, windowProperties_.rows);
			// create the VT100 decoder, and associated PTY backend
		}





	private:

		/** Name of the session. */
		std::string name_;

		/** The associated terminal. 
		 */
		vterm::Terminal::Terminal * terminal_;

		/** Window associated with the terminal. 
		 */
		TerminalWindow * window_;

		/** Properties of the attached window. 
		 */
		TerminalWindow::Properties windowProperties_;

		/** Command to be executed for the terminal. 
		 */
		helpers::Command command_;


	};



} // namespace tpp
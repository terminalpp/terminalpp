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


	};



} // namespace tpp
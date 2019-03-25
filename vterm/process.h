#pragma once

#include "vterm.h"

namespace vterm {

	/** The class responsible for providing the contents of the virtual terminal and reacting to its events. 

	    The pairing between terminals and their processes must be exactly one to one. 
	 */
	class Process {
	public:

		/** Returns the terminal corresponding to the process. 
		 */
		VTerm * terminal() {
			return terminal_;
		}

		void attachTerminal(VTerm * terminal) {
			if (terminal_ == terminal)
				return;
			ASSERT(terminal_ == nullptr);
			terminal->attachProcess(this);
		}

		/** Detaches the process from its current terminal. 
		 */
		void detachTerminal() {
			ASSERT(terminal_ != nullptr);
			ASSERT(terminal_->process_ == this);
			terminal_->detachProcess();
		}

		virtual ~Process() {
			if (terminal_ != nullptr)
				detachTerminal();
		}

	protected:

		/** Called when attached terminal changes size. 
		 */
		virtual void resize(unsigned cols, unsigned rows) = 0;

		// TODO handle terminal events (terminal will call these methods when the events happen)

	private:
		friend class VTerm;

		VTerm * terminal_;

	}; // vterm::Process

	/** The piped process provides an implementation of a simple buffer that allows /
	class PipedProcess : public Process {

	}; // vterm::PipedProcess

} // namespace vterm
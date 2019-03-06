# pragma once

#include "virtual_terminal.h"

namespace vterm {

	/** Terminal connector is basic encapsulation of object responsible for feeding the terminal with data to display that is able to react to the terminal events.

		The most straightforward connector is a wrapper around executable which simply passes the output of the executable to the terminal and sends the terminal events to the input stream of the attached executable, but other connectors are possible as well (such as the UI root object).
	 */
	class VirtualTerminal::Connector {
	public:

		/** Returns the terminakl to which the connector outputs. 
		 */
		VirtualTerminal * terminal() {
			return terminal_;
		}

		/** Sets the terminal to which the connector will output data and read events from. 
		 */
		void setTerminal(VirtualTerminal * terminal) {
			terminal_ = terminal;
		}

	protected:

		Connector() :
			terminal_{ nullptr } {
		}

		/** Called when the attached terminal changes its dimensions.
		 */
		virtual void resize(unsigned width, unsigned height) = 0;

	private:
		VirtualTerminal * terminal_;
	};

} // namespace vterm
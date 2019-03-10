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
			terminal_{ nullptr },
			buffer_{ new char[512] },
			bufferSize_{ 512 },
			bufferWrite_{ 0 } {
		}

		/** Called when the attached terminal changes its dimensions.
		 */
		virtual void resize(unsigned width, unsigned height) = 0;

		char * getWriteBuffer(unsigned & size) {
			size = bufferSize_ - bufferWrite_;
			return buffer_ + bufferWrite_;
		}

		void writeBytes(unsigned size) {
			if (size == 0)
				return;
			bufferWrite_ += size;
			ASSERT(bufferWrite_ <= bufferSize_) << "Buffer overrun - use getWriteBuffer()";
			unsigned bytesProcessed = processBytes(buffer_, bufferWrite_);
			if (bytesProcessed == bufferWrite_) {
				bufferWrite_ = 0;
			} else {
				memcpy(buffer_, buffer_ + bytesProcessed, bufferWrite_ - bytesProcessed);
				bufferWrite_ -= bytesProcessed;
			}
		}

		virtual unsigned processBytes(char * buffer, unsigned size) = 0;

		void repaint(unsigned left, unsigned top, unsigned cols, unsigned rows) {
			if (terminal_ != nullptr)
				terminal_->repaint(left, top, cols, rows);

		}

	private:
		VirtualTerminal * terminal_;

		char * buffer_;
		unsigned bufferSize_;
		unsigned bufferWrite_;

	};

} // namespace vterm
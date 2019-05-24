
#include <cstring>

#include "terminal.h"
#include "pty.h"

namespace vterm {

	void Terminal::PTYBackend::resize(unsigned cols, unsigned rows) {
		if (pty_ != nullptr)
			pty_->resize(cols, rows);
	}

	bool Terminal::PTYBackend::waitForInput() {
		// if there is available data already, return
		if (available_)
			return true;
		ASSERT(writeStart_ < (buffer_ + bufferSize_)) << "Buffer full";
		// otherwise read from the attached pty 
		while (true) {
			size_t size = pty_->receiveData(writeStart_, buffer_ + bufferSize_ - writeStart_);
			if (size == 0)
				return false;
			writeStart_ += size;
			available_ = true;
			return true;
		}
	}

	void Terminal::PTYBackend::processInput() {
		if (!available_)
			return;
		// process the data received
		size_t processed = dataReceived(buffer_, writeStart_ - buffer_);
		// if not all data has been processed, copy the unprocessed data to the beginning of the buffer
		if (buffer_ + processed != writeStart_)
			memcpy(buffer_, buffer_ + processed, writeStart_ - (buffer_ + processed));
		writeStart_ -= processed;
		// finally set available to false so that next wait cycle can be initiated
		available_ = false;
	}

	/*
	void Terminal::PTYBackend::processInput(bool wait) {
		if (!pty_)
			return;
		if (!wait && pty_->receiveDataReady() == 0)
			return;
		ASSERT(writeStart_ < (buffer_ + bufferSize_)) << "Buffer full";
		// get more data
		size_t size = pty_->receiveData(writeStart_, buffer_ + bufferSize_ - writeStart_);
		// TODO can we assume this is EOF and how to communicate this nicely?
		if (size == 0)
			return;
		// update size to include the writeStart_ offset so that it means all available data from the beginning of the buffer
		size = size + (writeStart_ - buffer_);
		// process the data together with any leftovers
		size_t processed = dataReceived(buffer_, size);
		// if not all was processed, copy leftovers to the beginning
		if (processed != size)
			// TODO - is this ok for memcpy? (overlapped, etc? )
			memcpy(buffer_, buffer_ + processed, size - processed);
		// set new start of writing after the leftovers (if any)		
		writeStart_ = buffer_ + (size - processed);
	} */

	size_t Terminal::PTYBackend::sendData(char const * buffer, size_t size) {
		ASSERT(pty_ != nullptr) << "Cannot send data when no PTY is attached";
		return pty_->sendData(buffer, size);
	}

	void Terminal::PTYBackend::resizeComBuffer(size_t newSize) {
		ASSERT(newSize >= static_cast<size_t>(writeStart_ - buffer_)) << "Not enough space for unprocessed data after resizing (unprocessed " << (writeStart_ - buffer_) << ", requested size " << newSize << ")";
		char* nb = new char[newSize];
		memcpy(nb, buffer_, writeStart_ - buffer_);
		writeStart_ = nb + (writeStart_ - buffer_);
		delete [] buffer_;
		buffer_ = nb;
	}

} // namespace vterm
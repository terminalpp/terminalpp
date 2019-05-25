
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
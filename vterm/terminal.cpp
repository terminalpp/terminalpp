
#include <cstring>

#include "terminal.h"
#include "pty.h"

namespace vterm {

	void Terminal::PTYBackend::setPty(PTY* value) {
		if (pty_ != value) {
			pty_ = value;
			if (pty_ != nullptr && terminal() != nullptr)
				pty_->resize(terminal()->cols(), terminal()->rows());
		}
	}

	void Terminal::PTYBackend::startThreadedReceiver() {
		std::thread t([this]() {
			while (true)
				receiveAndProcessData();
			});
		t.detach();
	}

	void Terminal::PTYBackend::resize(unsigned cols, unsigned rows) {
		if (pty_ != nullptr)
			pty_->resize(cols, rows);
	}

	size_t Terminal::PTYBackend::sendData(char const * buffer, size_t size) {
		ASSERT(pty_ != nullptr) << "Cannot send data when no PTY is attached";
		return pty_->sendData(buffer, size);
	}

	void Terminal::PTYBackend::resizeBuffer(size_t newSize) {
		ASSERT(newSize >= (writeStart_ - buffer_)) << "Not enough space for unprocessed data after resizing (unprocessed " << (writeStart_ - buffer_) << ", requested size " << newSize << ")";
		char* nb = new char[newSize];
		memcpy(nb, buffer_, writeStart_ - buffer_);
		writeStart_ = nb + (writeStart_ - buffer_);
		delete [] buffer_;
		buffer_ = nb;
	}

	void Terminal::PTYBackend::receiveAndProcessData() {
		ASSERT(pty_ != nullptr) << "Cannot receive data, PTY missing";
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
	}

} // namespace vterm
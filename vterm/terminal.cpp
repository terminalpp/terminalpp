
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
		writeStart_ += size;
		// process the data together with any leftovers
		size = size - dataReceived(buffer_, writeStart_ - buffer_);
		// if not all was processed, copy leftovers to the beginning
		if (buffer_ + size != writeStart_) 
			// TODO - is this ok for memcpy? (overlapped, etc? )
			memcpy(buffer_, buffer_ + size, writeStart_ - buffer_ - size);
		// set new start of writing after the leftovers (if any)		
		writeStart_ = buffer_ + size;
	}

	
#ifdef HAHA

	// Terminal --------------------------------------------------------------------------------------

	Terminal::Terminal(unsigned cols, unsigned rows) :
		cols_(cols),
		rows_(rows),
		cursorPos_(0,0),
		cursorCharacter_(0x2581),
		cursorVisible_(true),
		cursorBlink_(true),
		cells_(new Cell[cols * rows]) {
	}

	Terminal::~Terminal() {
		delete [] cells_;
	}


	void Terminal::doResize(unsigned cols, unsigned rows) {
		cols_ = cols;
		rows_ = rows;
		// resize the layers.
		delete [] cells_;
		cells_ = new Cell[cols * rows];
	}

	// IOTerminal ------------------------------------------------------------------------------------

	IOTerminal::~IOTerminal() {
		if (inputBuffer_ != nullptr) {
			inputReader_.join();
			delete[] inputBuffer_;
		}
	}

	void IOTerminal::doStart() {
		inputBuffer_ = new char[inputBufferSize_];
		inputReader_ = std::thread(IOTerminal::InputStreamReader, this);
	}

	void IOTerminal::InputStreamReader(IOTerminal * terminal) {
		char * buffer = terminal->inputBuffer_;
		size_t bufferSize = terminal->inputBufferSize_;
		size_t writeOffset = 0;
		size_t processedSize = 0;
		size_t size = bufferSize;
		while (terminal->readInputStream(buffer + writeOffset, size)) {
			// size now contains the number of bytes written, calculate expected processed size and process
			processedSize = writeOffset + size;
			terminal->processInputStream(buffer, processedSize);
			// if not everything was processed, copy the end of the buffer to the beinning and update the writeOffset accordingly
			writeOffset = writeOffset + size - processedSize;
			if (writeOffset > 0) {
				memcpy(buffer, buffer + processedSize, writeOffset);
				size = bufferSize - writeOffset;
				// otherwise writeOffset is 0, size is bufferSize meaning all buffer can be filled
			}
			else {
				size = bufferSize;
			}
		}
	}

#endif

} // namespace vterm
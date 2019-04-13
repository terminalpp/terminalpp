
#include "terminal.h"

namespace vterm {

	// Terminal --------------------------------------------------------------------------------------

	Terminal::Terminal(unsigned cols, unsigned rows) :
		cols_(cols),
		rows_(rows),
		cursorPos_(0,0),
		defaultLayer_(new LayerImpl(this)) {
	}

	Terminal::~Terminal() {
		delete defaultLayer_;
	}


	void Terminal::doResize(unsigned cols, unsigned rows) {
		cols_ = cols;
		rows_ = rows;
		// resize the layers. 
		defaultLayer_->resize(cols, rows);
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

} // namespace vterm
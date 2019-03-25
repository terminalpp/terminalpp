#include "terminal.h"

namespace vterm {

	// Terminal --------------------------------------------------------------------------------------

	Terminal::Terminal(unsigned cols, unsigned rows) :
		cols_(cols),
		rows_(rows),
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

	// PTYTerminal -----------------------------------------------------------------------------------


	void PTYTerminal::doResize(unsigned cols, unsigned rows) {
		Terminal::doResize(cols, rows);
		if (process_)
			process_->resize(cols, rows);
	}

	void PTYTerminal::attachProcess(Process * process) {
		ASSERT(process_ == nullptr);
		process_ = process;
		process_->terminal_ = this;
		std::lock_guard<std::mutex> g(m_);
		process_->resize(cols(), rows());
	}

	// PTYTerminal::Process --------------------------------------------------------------------------

	PTYTerminal::Process::Process(PTYTerminal * terminal, size_t bufferSize) :
		terminal_(terminal),
		inputBufferRead_(new char[bufferSize]),
		inputBufferWrite_(inputBufferRead_),
		inputBufferEnd_(inputBufferRead_ + bufferSize) {
		ASSERT(terminal_ != nullptr);
		terminal_->attachProcess(this);
	}

	PTYTerminal::Process::~Process() {
		// detach from the terminal
		if (terminal_ != nullptr)
		    terminal_->process_ = nullptr;
		// delete the buffers
		delete [] inputBufferRead_;
	}

	void PTYTerminal::Process::commitInputBuffer(char * buffer, size_t size) {
		ASSERT(buffer == inputBufferWrite_);
		// determine the expected end of processed buffer, including any previously unprocessed items
		size_t processedBytes = size + (buffer - inputBufferRead_);
		// call the attached terminal with the buffer and size
		terminal_->processInput(inputBufferRead_, processedBytes);
		// if fewer bytes were processed, we must copy the unprocessed ones to the beginning of the buffer and adjust the buffer write pointer accordingly
		if (inputBufferRead_ + processedBytes < buffer + size) {
			inputBufferWrite_ = inputBufferRead_ + ((buffer + size) - (inputBufferRead_ + processedBytes));
			memcpy(inputBufferRead_, inputBufferRead_ + processedBytes, inputBufferWrite_ - inputBufferRead_);
		// otherwise next write may use the full buffer
		} else {
			inputBufferWrite_ = inputBufferRead_;
		}
	}

} // namespace vterm
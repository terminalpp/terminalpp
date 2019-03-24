#include "vterm.h"

namespace vterm {

	/** The size of the new terminal must be specified, a process may be specified as well, in which case the terminal attaches to the process. 
	 */
	VTerm::VTerm(unsigned cols, unsigned rows, VTerm::Process * process):
		cells_(new Cell[cols * rows]),
		cols_(cols),
		rows_(rows),
		process_(nullptr) {
		if (process != nullptr)
			attachProcess(process);
	}

	VTerm::~VTerm() {
		detachProcess();
		delete cells_;
	}

	/** Getting the columns and rows alone is not supported to make sure there can't be a resize change between obtaining the two numbers. 
     */
	Dimension VTerm::size() const {
		std::lock_guard<std::mutex> g(const_cast<VTerm*>(this)->m_);
		return Dimension{ cols_, rows_ };
	}

	void VTerm::setProcess(VTerm::Process * process) {
		std::lock_guard<std::mutex> g(m_);
		if (process_ == process)
			return;
		detachProcess();
		attachProcess(process);
	}

	VTerm::Screen VTerm::getScreen() {
		return Screen(this);
	}

	VTerm::Screen const VTerm::getScreen() const {
		return Screen(const_cast<VTerm*>(this));
	}
	
	void VTerm::resize(unsigned cols, unsigned rows) {
		// don't do anything if the new and old size matches
		if (cols_ == cols && rows_ == rows)
			return;
		// obtain the lock on the buffer and peform the modification
		{
			std::lock_guard<std::mutex> g(m_);
			cols_ = cols;
			rows_ = rows;
			// change the size of the cells array
			delete[] cells_;
			cells_ = new Cell[cols * rows];
		}
		// resize the connector, if valid
		if (process_ != nullptr)
			process_->resize(cols, rows);
		// trigger the event
		trigger(onResize, Dimension{ cols, rows });
	}

	void VTerm::attachProcess(VTerm::Process * process) {
		if (process != nullptr) {
			ASSERT(process_ == nullptr) << "Terminal already has a process attached, detach first";
			ASSERT(process->terminal_ == nullptr) << "Process already attached.";
			process_ = process;
			process_->terminal_ = this;
			process_->resize(cols_, rows_);
		}
	}

	void VTerm::detachProcess() {
		if (process_ != nullptr) {
			process_->terminal_ = nullptr;
			process_ = nullptr;
		}
	}

	VTerm::Process::Process(size_t bufferSize) :
		bufferBegin_(new char[bufferSize]),
		readStart_(bufferBegin_),
		writeStart_(bufferBegin_),
		bufferEnd_(bufferBegin_ + bufferSize) {
	}

	void VTerm::Process::commitInputBuffer(char * buffer, size_t size) {
		// TODO what to do if terminal is not attached? 
		ASSERT(terminal_ != nullptr); 
		ASSERT(buffer == writeStart_);
		char * end = buffer + size;
		// convert the size if there were any data left from last 
		size = size + (writeStart_ - readStart_);
		// process the received data
		terminal_->processInput(readStart_, size);
		readStart_ = bufferBegin_;
		// if not everything has been processed, copy the unprocessed part at the beginning of the buffer
		char * processedEnd = readStart_ + size;
		if (processedEnd == end) {
			writeStart_ = bufferBegin_;
		} else {
			memcpy(readStart_, processedEnd, end - processedEnd);
			writeStart_ = bufferBegin_ + (end - processedEnd);
		}
	}

} // namespace vterm
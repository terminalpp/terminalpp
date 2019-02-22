#pragma once

#include <mutex>

#include "helpers/object.h"

#include "screen_buffer.h"

namespace vterm {
	using namespace helpers;

	struct TerminalSize {
		unsigned cols;
		unsigned rows;
	};

	typedef EventPayload<void, Object> ChangeEvent;
	typedef EventPayload<TerminalSize, Object> ResizeEvent;

	/** Virtual terminal class. 

	 */
	class VirtualTerminal : public Object {
	public:

		VirtualTerminal() :
			cols_(0),
			rows_(0),
			buffer_(nullptr) {
		}

		/** Holder for rendering information of a single cell. 
		 */
		class ScreenCell;

		/** The screen buffer as exported by the terminal.
		 */
		class ScreenBuffer;

		/** Returns the current width of the terminal 
		 */
		unsigned cols() const {
			return cols_;
		}

		/** Returns the current height of the terminal. 
		 */
		unsigned rows() const {
			return rows_;
		}

		/** Resizes the virtual terminal. 
		 */
		void resize(unsigned cols, unsigned rows);

		/** Returns the screen buffer of the terminal so that in can be read or written. 
		 */
		ScreenBuffer screenBuffer();


		/** Triggers for an otherwise unspecified change of the terminal. 
		 */
		Event<ChangeEvent> onChange;

		/** Triggers when the terminal has been resized. 
		 */
		Event<ResizeEvent> onResize;

	private:
		unsigned cols_;
		unsigned rows_;
		ScreenCell * buffer_;
		std::mutex bufferLock_;
	}; // vterm::VirtualTerminal

	/** Although quite a lot memory is required for each cell, this should be perfectly fine since we need only a very small ammount of cells for any terminal window.
	 */
	class VirtualTerminal::ScreenCell {
	public:
		/** Foreground - text color.
		 */
		Color fg;

		/** Background color.
		 */
		Color bg;

		/** The character in the cell.
		 */
		Char c;

		/** The font to be used for displaying the cell.
		 */
		Font font;
	}; // vterm::VirtualTerminal::ScreenCell


	/** The virtual terminal's screen can be read and written to using the screen buffer. Obtaining the screen buffer object locks the screen buffer so it shoul only be held for the minimal necessary time.
	 */
	class VirtualTerminal::ScreenBuffer {
	public:

		/** Returns the screen buffer's width.
		 */
		unsigned cols() const {
			return terminal_->cols_;
		}

		/** Returns the screen buffer's height.
		 */
		unsigned rows() const {
			return terminal_->rows_;
		}

		ScreenCell const & at(unsigned col, unsigned row) const {
			ASSERT(col < cols() && row < rows()) << "Indices " << col << ";" << row << " out of bounds " << cols() << ";" << rows();
			return terminal_->buffer_[row * cols_ + col];
		}

		ScreenCell & at(unsigned col, unsigned row) const {
			ASSERT(col < cols() && row < rows()) << "Indices " << col << ";" << row << " out of bounds " << cols() << ";" << rows();
			return terminal_->buffer_[row * cols_ + col];
		}

		ScreenBuffer(ScreenBuffer && other) :
			terminal_(other.terminal_) {
			other.terminal_ = nullptr;
		}

		ScreenBuffer() = delete;
		ScreenBuffer(ScreenBuffer &) = delete;
		ScreenBuffer & operator = (ScreenBuffer &) = delete;

		/** When the buffer is deleted, it releases the mutex.
		 */
		~ScreenBuffer() {
			terminal_->bufferLock_.unlock();
			terminal_ = nullptr;
		}

	private:
		friend class VirtualTerminal;

		ScreenBuffer(VirtualTerminal * terminal) :
			terminal_(terminal) {
		}

		VirtualTerminal * terminal_;

	}; // vterm::VirtualTerminal::ScreenBuffer

	/** Locks the virtual terminal's screen buffer access before returning. The destructor of the screen buffer object will release the lock when no longer needed. 
	 */
	inline VirtualTerminal::ScreenBuffer VirtualTerminal::screenBuffer() {
		bufferLock_.lock();
		return ScreenBuffer(this);
	}

	/** 
	 */
	void VirtualTerminal::resize(unsigned cols, unsigned rows) {
		if (cols == cols_ && rows == rows_)
			return;
		ScreenCell * newBuffer = new ScreenCell[cols * rows];
		// TODO do we want to copy the old buffer parts to the new buffer so that something can be displayed, or is it ok to wait for the connector to deliver new content based on the resize?
		ScreenCell * oldBuffer = buffer_;
		{
			std::lock_guard<std::mutex> g(bufferMutex_);
			buffer_ = newBuffer_;
		}
		delete[] buffer_;
		onResize.trigger(TerminalResize{ cols, rows }); */
	}

} // namespace vterm
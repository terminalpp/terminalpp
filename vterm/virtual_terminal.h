#pragma once

#include <mutex>

#include "helpers/object.h"

#include "color.h"
#include "font.h"
#include "char.h"

namespace vterm {
	using namespace helpers;

	struct TerminalSize {
		unsigned cols;
		unsigned rows;
	};

	struct TerminalRepaint {
		unsigned left;
		unsigned top;
		unsigned cols;
		unsigned rows;
	};

	typedef EventPayload<void, Object> ChangeEvent;
	typedef EventPayload<TerminalSize, Object> ResizeEvent;
	typedef EventPayload<TerminalRepaint, Object> RepaintEvent;

	/** Virtual terminal class. 

	 */
	class VirtualTerminal : public Object {
	public:

		class Connector;

		/** Holder for rendering information of a single cell.

			Although quite a lot memory is required for each cell, this should be perfectly fine since we need only a very small ammount of cells for any terminal window.
		 */
		class ScreenCell {
		public:
			/** Foreground - text color.
			 */
			Color fg;

			/** Background color.
			 */
			Color bg;

			/** The character in the cell.
			 */
			Char::UTF16 c;

			/** The font to be used for displaying the cell.
			 */
			Font font;
		}; // vterm::VirtualTerminal::ScreenCell


		/** The screen buffer as exported by the terminal.

			The virtual terminal's screen can be read and written to using the screen buffer. Obtaining the screen buffer object locks the screen buffer so it shoul only be held for the minimal necessary time.
		 */
		class ScreenBuffer {
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
				return terminal_->buffer_[row * cols() + col];
			}

			ScreenCell & at(unsigned col, unsigned row) {
				ASSERT(col < cols() && row < rows()) << "Indices " << col << ";" << row << " out of bounds " << cols() << ";" << rows();
				return terminal_->buffer_[row * cols() + col];
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

		VirtualTerminal() :
			cols_(0),
			rows_(0),
			buffer_(nullptr) {
		}

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
		void resize(unsigned cols, unsigned rows) {
			if (cols == cols_ && rows == rows_)
				return;
			ScreenCell * newBuffer = new ScreenCell[cols * rows];
			// TODO do we want to copy the old buffer parts to the new buffer so that something can be displayed, or is it ok to wait for the connector to deliver new content based on the resize?
			ScreenCell * oldBuffer = buffer_;
			{
				std::lock_guard<std::mutex> g(bufferLock_);
				buffer_ = newBuffer;
				cols_ = cols;
				rows_ = rows;
				for (unsigned r = 0; r < rows; ++r) {
					char i = 0;
					for (unsigned c = 0; c < cols; ++c) {
						buffer_[r * cols + c].c = (unsigned)('0' + (i++ % 10));
						buffer_[r * cols + c].fg = Color::White;
						buffer_[r * cols + c].bg = Color::Black;
					}
				}
			}
			delete[] oldBuffer;
			// trigger the onResize event
			trigger(onResize, TerminalSize{ cols, rows });
			trigger(onRepaint, TerminalRepaint{ 0,0,cols, rows });
		}

		/** Returns the screen buffer of the terminal so that in can be read or written. 
		    
			Locks the virtual terminal's screen buffer access before returning. The destructor of the screen buffer object will release the lock when no longer needed.
		 */
		ScreenBuffer screenBuffer() {
			bufferLock_.lock();
			return ScreenBuffer(this);
		}


		/** Triggers for an otherwise unspecified change of the terminal. 
		 */
		Event<ChangeEvent> onChange;

		/** Triggers when the terminal has been resized. 
		 */
		Event<ResizeEvent> onResize;

		/** Triggered when the data in the terminal change. 
		 */
		Event<RepaintEvent> onRepaint;

	private:
		unsigned cols_;
		unsigned rows_;
		ScreenCell * buffer_;
		std::mutex bufferLock_;
	}; // vterm::VirtualTerminal



	/** 
	 */
	class VirtualTerminal::Connector {
	public:

		VirtualTerminal const * terminal() const {
			return terminal_;
		}

		VirtualTerminal * terminal() {
			return terminal_;
		}

		void setTerminal(VirtualTerminal * terminal) {
			terminal_ = terminal;
		}
	protected:

		virtual void resize(unsigned width, unsigned height) = 0;


	private:
		VirtualTerminal * terminal_;
	};


} // namespace vterm
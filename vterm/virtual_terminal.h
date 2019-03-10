#pragma once

#include <mutex>

#include "helpers/object.h"

#include "color.h"
#include "font.h"
#include "char.h"
#include "key.h"

namespace vterm {
	using namespace helpers;

	/**  Payload for terminal resize event. 

	     Specifies the new (current at the time the event is triggered) size of the terminal in number of columns and rows. 
	 */
	struct TerminalSize {
		unsigned cols;
		unsigned rows;
	};

	/** Payload for terminal repaint event. 

	    Specifies the region of the terminal that should be repainted. 

		TODO should use rectangle? 
	 */
	struct TerminalRepaint {
		unsigned left;
		unsigned top;
		unsigned cols;
		unsigned rows;
	};

	/** Payload for terminal key events. 

	    Determines the type of the event (KeyUp, KeyDown, KeyPress), and the key itself. 
	 */
	struct TerminalKey {
		enum class Kind {
			KeyDown, 
			KeyUp,
			KeyPress,
		};
		Kind kind;
		Key key;
	};

	/** Payload for terminal mouse events. 

	    Contains the mouse coordinates and state of mouse buttons. 
	 */
	struct TerminalMouse {
		unsigned col;
		unsigned row;
		unsigned char buttons;
	};

	typedef EventPayload<void, Object> ChangeEvent;
	typedef EventPayload<TerminalSize, Object> ResizeEvent;
	typedef EventPayload<TerminalRepaint, Object> RepaintEvent;
	typedef EventPayload<TerminalKey, Object> KeyEvent;
	typedef EventPayload<TerminalMouse, Object> MouseEvent;


	/** Virtual terminal.

	    Character = 4 bytes
		Foreground Color = 3 bytes
		Background Color = 3 bytes
		Font = 2 bytes


	 */
	class VirtualTerminal : public Object {
	public:

		class Renderer;
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
			Char::UTF8 c;

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
			buffer_(nullptr),
		    renderer_(nullptr) {
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
		void resize(unsigned cols, unsigned rows);

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

	protected:
		/** Detaches the given renderer from the virtual terminal. 
		 */
		void detachRenderer();

		/** Attaches to the provided renderer. 

		    Updates its size according to the new renderer's dimensions.
		 */
		void attachRenderer(Renderer * renderer);

		void repaint(unsigned left, unsigned top, unsigned cols, unsigned rows);

	private:
		unsigned cols_;
		unsigned rows_;
		ScreenCell * buffer_;
		Renderer * renderer_;
		std::mutex bufferLock_;
	}; // vterm::VirtualTerminal


} // namespace vterm

#include "vterm_renderer.h"
#include "vterm_connector.h"
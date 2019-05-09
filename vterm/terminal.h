#pragma once

#include <thread>
#include <atomic>
#include <mutex>
#include <string>
#include <list>

#include "helpers/object.h"
#include "helpers/shapes.h"

#include "color.h"
#include "font.h"
#include "char.h"
#include "key.h"

namespace vterm {

	/** Describes a single character cell of the terminal. 

	    Each cell may specify its own font and attributes, text and background color and a character to display. 
	 */
	class Cell {
	public:
		/** The font to be used to render the cell.
		 */
		Font font;

		/** Text color.
		 */
		Color fg;

		/** Background color.
		 */
		Color bg;

		/** Character to be displayed (utf8).
		 */
		Char::UTF8 c;

		/** Determines if the cell is dirty, i.e. it should be redrawn. 
		 */
		bool dirty;

		/** Default constructor for a cell created white space on a black background.
		 */
		Cell() :
			fg(Color::White()),
			bg(Color::Black()),
			c(' '),
		    dirty(false) {
		}

	}; // vterm::Cell

	/** Virtual Terminal Implementation. 

	    The terminal only implements the storage and programmatic manipulation of the terminal data and does not concern itself with either making the contents visible to the user (this is the job of terminal renderer), or by specifying ways on how & when the contents of the terminal should be updated, which is a job of classes that inherit from the virtual terminal (such as PTYTerminal). 
	 */
	class Terminal : public helpers::Object {
	public:

		/** Provides access to the terminal's buffer and associated features. 
		 */
		class Buffer {
		public:
			/** Returns the terminal associated with the buffer. 
			 */
			Terminal * terminal() {
				return terminal_;
			}

			Terminal const* terminal() const {
				return terminal_;
			}

			/* Returns the width and height of the buffer (and the terminal). 
			 */

			unsigned cols() const {
				return terminal_->cols_;
			}

			unsigned rows() const {
				return terminal_->rows_;
			}

			/** Returns the given cell of the terminal's buffer. 
			 */
			Cell & at(unsigned col, unsigned row) {
				ASSERT(col < cols() && row < rows());
				return terminal_->cells_[row * cols() + col];
			}

			Cell & at(helpers::Point const& p) {
				return at(p.col, p.row);
			}

			Cell const & at(unsigned col, unsigned row) const {
				ASSERT(col < cols() && row < rows());
				return terminal_->cells_[row * cols() + col];
			}

			Cell const & at(helpers::Point const& p) const {
				return at(p.col, p.row);
			}

			// constructors, assignments

			Buffer(Terminal* t) :
				terminal_(t) {
				terminal_->m_.lock();
			}

			Buffer(Buffer&& from) :
				terminal_(from.terminal_) {
				from.terminal_ = nullptr;
			}

			Buffer(Buffer const&) = delete;

			~Buffer() {
				if (terminal_ != nullptr)
					terminal_->m_.unlock();
			}

		private:
			Terminal * terminal_;
		}; // Terminal::Buffer

		// events ---------------------------------------------------------------------------------

		/** Size of the terminal (in cell columns and rows).
		 */
		struct Size {
			/** Number of columns. */
			unsigned cols;
			/** Number of rows. */
			unsigned rows;

			Size(unsigned cols, unsigned rows) :
				cols(cols),
				rows(rows) {
			}
		};

		typedef helpers::EventPayload<Size, helpers::Object> ResizeEvent;
		typedef helpers::EventPayload<void, helpers::Object> RepaintEvent;

		/** Triggered when the terminal is resized.
		 */
		helpers::Event<ResizeEvent> onResize;

		/** Triggered when the terminal contents change and should be repainted.
		 */
		helpers::Event<RepaintEvent> onRepaint;

		// properties -----------------------------------------------------------------------------

		/** Returns the size of the terminal.

			Separate access to rows and columns is not permitted because should the terminal size change in between the calls, inconsistent results would be returned.
		 */
		Size size() const {
			std::lock_guard<std::mutex> g(m_);
			return Size{ cols_, rows_ };
		}

		/** Returns the cursor coordinates. 
		 */
		helpers::Point cursorPos() const {
			return cursorPos_;
		}

		Char::UTF8 cursorCharacter() const {
			return cursorCharacter_;
		}

		bool cursorVisible() const {
			return cursorVisible_;
		}

		bool cursorBlink() const {
			return cursorBlink_;
		}


		// methods --------------------------------------------------------------------------------

		virtual void keyDown(Key k) = 0;
		virtual void keyUp(Key k) = 0;
		virtual void sendChar(Char::UTF8 c) = 0;

        /** Call when paste from clipboard has been issued in the terminal window. 
         */
        virtual void paste(std::string const & what) = 0;

		/** Resizes the terminal to given size.
		 */
		bool resize(unsigned cols, unsigned rows) {
			{
				std::lock_guard<std::mutex> g(m_);
				// don't do anything if the size is the same
				if (cols_ == cols && rows_ == rows)
					return false;
				// perform the actual resize operation
				doResize(cols, rows);
			}
			// after the lock has been released, emit the onResize event
			trigger(onResize, Size{ cols, rows });
			return true;
		}

		/** Returns the contents of the default layer.

			The returned layer smart pointer locks the whole terminal for reads or updates of the screen and its size so the layer should be kept around for as little time as needed (typically to update its contents, or render it).
		 */
		Buffer getBuffer() {
			return Buffer(this);
		}

		// constructors & destructors -------------------------------------------------------------

		/** Deletes the virtual terminal.
		 */
		~Terminal() override;

	protected:

		/** Creates the virtual terminal of given size.
		 */
		Terminal(unsigned cols, unsigned rows);

		void repaint() {
    		trigger(onRepaint);
		}

		/** Actually resizes the terminal.

		    The called code may assume that the terminal is locked. 
		 */
		virtual void doResize(unsigned cols, unsigned rows);

		/** Access to the terminal's buffer without the automatically locking buffer. 
		 */
		Cell const& at(unsigned col, unsigned row) const {
			ASSERT(col < cols_ && row < rows_);
			return cells_[row * cols_ + col];
		}

		Cell& at(unsigned col, unsigned row) {
			ASSERT(col < cols_ && row < rows_);
			return cells_[row * cols_ + col];
		}

		/** Mutex for protecting read and write access to the terminal contents to make sure that all requests will return consistent data.
		 */
		mutable std::mutex m_;

		/** Number of columns the terminal stores in each layer. */
		unsigned cols_;

		/** Number of rows the terminal stores in each layer. */
		unsigned rows_;

		/** Position of the cursor. 
		 */
		helpers::Point cursorPos_;

		Char::UTF8 cursorCharacter_;

		bool cursorVisible_;

		bool cursorBlink_;

		/** Actual contents of the terminal buffer. 
		 */
		Cell * cells_;

	private:


	}; // vterm::VTerm


	/** Terminal extension to allow communication over input output streams. 
	 */
	class IOTerminal : public Terminal {
	public:
		static constexpr size_t DEFAULT_BUFFER_SIZE = 1024;

	protected:
		IOTerminal(unsigned cols, unsigned rows, size_t bufferSize = DEFAULT_BUFFER_SIZE) :
			Terminal(cols, rows),
			inputBuffer_(nullptr),
		    inputBufferSize_ (bufferSize) {
		}

		~IOTerminal() override;

		virtual void doStart();

		/** Called when new data has been received on the input stream. 
		 */
		virtual void processInputStream(char * buffer, size_t & size) = 0;

		/** Called by the reader thread when new data from the input stream should be read. 
		 */
		virtual bool readInputStream(char * buffer, size_t & size) = 0;

		/** Writes the given buffer to the output, i.e. sends the data to the connected application. 
		 */
		virtual bool write(char const * buffer, size_t size) = 0;

	private:

		static void InputStreamReader(IOTerminal * terminal);

		std::thread inputReader_;

		char * inputBuffer_;

		size_t inputBufferSize_;

	};

} // namespace vterm

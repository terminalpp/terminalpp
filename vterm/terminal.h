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
#include "mouse.h"


namespace vterm {

	class PTY;

	/** Implementation of the terminal. 

	    The terminal provides encapsulation over the screen buffer and provides the communication between the frontend and backend. The terminal frontend is responsible for rendering the contents of the terminal to the user and sending the terminal the user input events. The backend of the terminal relays the user input events to the underlying process and reads from the process updates to the terminal state and stores them. 
	 */
	class Terminal : public helpers::Object {
	public:

		typedef helpers::EventPayload<void, helpers::Object> RepaintEvent;
		typedef helpers::EventPayload<std::string, helpers::Object> TitleChangeEvent;
		typedef helpers::EventPayload<int, helpers::Object> TerminationEvent;

		/** Cursor information. 
		 */
		class Cursor {
		public:
			/** Cursor column. 
			 */
			unsigned col;

			/** Cursor row. 
			 */
			unsigned row;

			/** Character of the cursor. 
			 */
			Char::UTF8 character;

			/** Color of the cursor (foreground)
			 */
			Color color;

			/** If true, the cursor should blink, if false, the cursor should be always on. 
			 */
			bool blink;

			/** If true, the cursor will be rendered by the terminal, if false, the cursor is not to be shown. 
			 */
			bool visible;

			Cursor() :
				col(0),
				row(0),
				character(0x2581), // underscore character
				color(Color::White()),
				blink(true),
				visible(true) {
			}

			//Cursor& operator = (Cursor const& other) = default;
		};

		/** Properties of a single cell of the terminal buffer.
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
		};

		class Buffer {
		public:

			/** Returns the columns and rows of the buffer.
			 */
			unsigned cols() const {
				return cols_;
			}

			unsigned rows() const {
				return rows_;
			}

			/** Returns the given cell of the terminal's buffer.
			 */
			Terminal::Cell& at(unsigned col, unsigned row) {
				ASSERT(col < cols_ && row < rows_);
				return cells_[row * cols_ + col];
			}

			Terminal::Cell const& at(unsigned col, unsigned row) const {
				ASSERT(col < cols_ && row < rows_);
				return cells_[row * cols_ + col];
			}

			Buffer(unsigned cols, unsigned rows) :
                cols_(cols),
                rows_(rows),
				cells_(new Cell[cols_ * rows_]) {
			}

            Buffer(Buffer && other) noexcept: 
                cols_(other.cols_),
                rows_(other.rows_),
                cells_(other.cells_) {
                    other.cols_ = 0;
                    other.rows_ = 0;
                    other.cells_ = nullptr;
                }

            Buffer & operator = (Buffer && other) noexcept {
                if (this != & other) {
                    delete[] cells_;
                    cols_ = other.cols_;
                    rows_ = other.rows_;
                    cells_ = other.cells_;
                    other.cols_ = 0;
                    other.rows_ = 0;
                    other.cells_ = nullptr;
                }
                return *this;
            }

            ~Buffer() {
				delete[] cells_;
			}

		private:

			friend class Terminal;
            friend class Backend;


			/** Resizes the buffer.
			 */
			void resize(unsigned cols, unsigned rows) {
				// TODO resizing the terminal should preserve some of the original context, if applicable
				delete[] cells_;
                cols_ = cols;
                rows_ = rows;
				cells_ = new Cell[cols_ * rows_];
			}

            unsigned cols_;

            unsigned rows_;

			/** The underlying array of cells.
			 */
			Cell* cells_;
		};

		class Renderer : public helpers::Object {
		public:

			Terminal* terminal() const {
				return terminal_;
			}

			void setTerminal(Terminal* terminal) {
				if (terminal_ != terminal) {
					detachFromTerminal();
					if (terminal != nullptr)
						attachToTerminal(terminal);
				}
			}

			unsigned cols() const {
				return cols_;
			}

			unsigned rows() const {
				return rows_;
			}

			virtual ~Renderer() {
				detachFromTerminal();
			}

		protected:

			/** Called by the attached terminal backend when new dirty cells exist which must be repainted.
             */
			virtual void repaint(RepaintEvent & e) = 0;

			virtual void titleChange(TitleChangeEvent& e) = 0;

			Renderer(unsigned cols, unsigned rows) :
				terminal_(nullptr),
				cols_(cols),
				rows_(rows) {
			}

			virtual void resize(unsigned cols, unsigned rows) {
				cols_ = cols;
				rows_ = rows;
				if (terminal_)
					terminal_->resize(cols, rows);
			}

			// keyboard events

			virtual void keyDown(Key k) {
				if (terminal_)
					terminal_->keyDown(k);
			}

			virtual void keyUp(Key k) {
				if (terminal_)
					terminal_->keyDown(k);
			}

			virtual void keyChar(Char::UTF8 c) {
				if (terminal_)
					terminal_->keyChar(c);
			}

			// mouse events

			virtual void mouseDown(unsigned col, unsigned row, MouseButton button) {
				if (terminal_)
					terminal_->mouseDown(col, row, button);
			}

			virtual void mouseUp(unsigned col, unsigned row, MouseButton button) {
				if (terminal_)
					terminal_->mouseUp(col, row, button);
			}

			virtual void mouseWheel(unsigned col, unsigned row, int by) {
				if (terminal_)
					terminal_->mouseWheel(col, row, by);
			}

			virtual void mouseMove(unsigned col, unsigned row) {
				if (terminal_)
					terminal_->mouseMove(col, row);
			}

			// clipboard events

			virtual void paste(std::string const& what) {
				if (terminal_)
					terminal_->paste(what);
			}

		private:


			virtual void detachFromTerminal() {
				if (terminal_ == nullptr)
					return;
				terminal_->onRepaint -= HANDLER(Renderer::repaint);
				terminal_->onTitleChange -= HANDLER(Renderer::titleChange);
				terminal_ = nullptr;
			}

			virtual void attachToTerminal(Terminal* terminal) {
				terminal_ = terminal;
				terminal->resize(cols_, rows_);
				terminal_->onRepaint += HANDLER(Renderer::repaint);
				terminal_->onTitleChange += HANDLER(Renderer::titleChange);
			}

			Terminal* terminal_;

			unsigned cols_;
			unsigned rows_;

		};

		class Backend {
		public:
			Terminal* terminal() const {
				return terminal_;
			}

		protected:

            void resizeBuffer(Buffer & buffer, unsigned cols, unsigned rows) {
                buffer.resize(cols, rows);
            }

			Backend() :
				terminal_(nullptr) {
			}

			/** Returns the current cursor information for the attached terminal so that it can be modified by the backend. 
			 */
			Cursor & cursor() {
				ASSERT(terminal_ != nullptr);
				return terminal_->cursor_;
			}

			unsigned cols() {
				ASSERT(terminal_ != nullptr);
				return terminal_->buffer().cols();
			}

			unsigned rows() {
				ASSERT(terminal_ != nullptr);
				return terminal_->buffer().rows();
			}

			Terminal::Buffer& buffer() {
				ASSERT(terminal_ != nullptr);
				return terminal_->buffer();
			}

			virtual void resize(unsigned cols, unsigned rows) = 0;

			virtual void keyDown(Key k) = 0;
			virtual void keyUp(Key k) = 0;
			virtual void keyChar(Char::UTF8 c) = 0;

			virtual void mouseDown(unsigned col, unsigned row, MouseButton button) = 0;
			virtual void mouseUp(unsigned col, unsigned row, MouseButton button) = 0;
			virtual void mouseWheel(unsigned col, unsigned row, int by) = 0;
			virtual void mouseMove(unsigned col, unsigned row) = 0;

			virtual void paste(std::string const & what) = 0;

			virtual ~Backend() {
				detachFromTerminal();
			}

		private:
			friend class Terminal;

			void detachFromTerminal() {
				if (terminal_ == nullptr)
					return;
				terminal_->backend_ = nullptr;
				terminal_ = nullptr;
			}

			/** Attaches the backend to specified terminal. 

			    Verifies that the terminal is unattached, links the terminal to the backend and then calls the resize function with the new terminal's size. 
			 */
			void attachToTerminal(Terminal* terminal) {
				terminal->backend_ = this;
				terminal_ = terminal;
				resize(terminal->cols(), terminal->rows());
			}

			Terminal* terminal_;
		};

		/** Backend which defines API for reading from / writing to PTY like deviced (i.e. input and output streams). 
		 */
		class PTYBackend : public Backend {
		public:

			/** Returns the pseudoterminal object associated with the backend. 
			 */
			PTY * pty() const {
				return pty_;
			}

			/** Waits for input to become available for processing, but does not process it right away. 
			 */
			bool waitForInput();

			/** If input is available, processes it, otherwise does nothing.  
			 */
			void processInput();

			~PTYBackend() override;

		protected:

			/** When the backend is resized, propagates the resize information to the underlying PTY, if present. 
			 */
			void resize(unsigned cols, unsigned rows) override;

			/** Creates a PTY backend. 

			    Note that the PTY is owned by the backend and when the backend will be deleted, so will the PTY. 
			 */
			PTYBackend(PTY * pty, size_t bufferSize = 10240) :
				pty_(pty),
				bufferSize_(bufferSize),
				buffer_(new char[bufferSize]),
				writeStart_(buffer_),
			    available_(false) {
			}

			/** Called when data is received.

				The first argument is the buffer containing the received data. The second argument is the size of available data from the buffer start. The function must return the number of bytes from the input actually processed. The remaining bytes will be prefetched to the next call to the dataReceived when more data has been sent by the client.
			 */
			virtual size_t dataReceived(char * buffer, size_t size) = 0;

			/** Sends given data using the attached PTY. 
			    
				Returns the number of bytes send which should be identical to the second argument (size) unless there was an I/O error while sending. 
			 */
			virtual size_t sendData(char const * buffer, size_t size);

			/** Resizes the internal buffer. 
			 */
			void resizeComBuffer(size_t newSize);

		private:
			PTY * pty_;

			size_t bufferSize_;
			char * buffer_;
			char * writeStart_;
			bool available_;
		};

		Terminal(unsigned cols, unsigned rows, Backend * backend = nullptr) :
			buffer_(cols, rows),
			backend_(backend) {
			if (backend_ != nullptr)
				backend_->attachToTerminal(this);
		}

		~Terminal() override {
			delete backend_;
		}

		// events

		helpers::Event<RepaintEvent> onRepaint;
		helpers::Event<TitleChangeEvent> onTitleChange;
		helpers::Event<TerminationEvent> onBackendTerminated;

		/** Returns the width and height of the terminal.
		 */
		unsigned cols() const {
			return buffer_.cols_;
		}

		unsigned rows() const {
			return buffer_.rows_;
		}

		/** Backend associated with the terminal. 

		    Note that if set, the backend is owned by the terminal, i.e. deleting the terminal will delete the backend as well. 
		 */
		Backend const * backend() const {
			return backend_;
		}

		void setBackend(Backend* backend) {
			if (backend_ != nullptr)
				NOT_IMPLEMENTED;
			backend_ = backend;
			backend->attachToTerminal(this);
		}

		/** Returns the buffer associated with the terminal. 
		 */
		Buffer const& buffer() const {
			return buffer_;
		}

		Buffer& buffer() {
			return buffer_;
		}

		Cursor const& cursor() const {
			return cursor_;
		}

		std::string const& title() const {
			return title_;
		}

		void setTitle(std::string const& value) {
			if (title_ != value) {
				title_ = value;
				trigger(onTitleChange, value);
			}
		}

		void resize(unsigned cols, unsigned rows) {
			if (buffer_.cols_ != cols || buffer_.rows_ != rows) {
				buffer_.resize(cols, rows);
				// update cursor position
				if (cursor_.col >= buffer_.cols_)
					cursor_.col = buffer_.cols_ - 1;
				if (cursor_.row >= buffer_.rows_)
					cursor_.row = buffer_.rows_ - 1;
				// pass to the backend
				if (backend_)
					backend_->resize(cols, rows);
			}
		}

		// keyboard events

		void keyDown(Key k) {
			if (backend_)
				backend_->keyDown(k);
		}

		void keyUp(Key k) {
			if (backend_)
				backend_->keyUp(k);
		}

		void keyChar(Char::UTF8 c) {
			if (backend_)
				backend_->keyChar(c);
		}

		// mouse events

		void mouseDown(unsigned col, unsigned row, MouseButton button) {
			if (backend_)
				backend_->mouseDown(col, row, button);
		}

		void mouseUp(unsigned col, unsigned row, MouseButton button) {
			if (backend_)
				backend_->mouseUp(col, row, button);
		}

		void mouseWheel(unsigned col, unsigned row, int by) {
			if (backend_)
				backend_->mouseWheel(col, row, by);
		}

		void mouseMove(unsigned col, unsigned row) {
			if (backend_)
				backend_->mouseMove(col, row);
		}

		// clipboard events

		void paste(std::string const& what) {
			if (backend_)
				backend_->paste(what);
		}

		/** When terminal repaint is requested, the onRepaint event is triggered. 
		 */
		void repaint() {
			trigger(onRepaint);
		}

	protected:

		/** Should be raised by the backend in case it is terminated. 
		 */
		void backendTerminated(int exitCode) {
			trigger(onBackendTerminated, exitCode);
		}

		Cursor cursor_;

	private:

		/** The buffer containing rendering information for all the cells. 
		 */
		Buffer buffer_;

		/** Backend attached to the terminal. 
		 */
		Backend* backend_;

		/** Title of the terminal. 
		 */
		std::string title_;

	};

} // namespace vterm

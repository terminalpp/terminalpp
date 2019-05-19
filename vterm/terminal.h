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

		/** Properties of a single cell of the terminal buffer.
		 */
		class Terminal::Cell {
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

		class Terminal::Buffer {
		public:

			/** Returns the terminal to which the buffer belongs.
			 */
			Terminal* terminal() const {
				return terminal_;
			}

			/** Returns the columns and rows of the buffer.
			 */
			unsigned cols() const {
				return terminal_->cols_;
			}

			unsigned rows() const {
				return terminal_->rows_;
			}

			/** Returns the given cell of the terminal's buffer.
			 */
			Terminal::Cell& at(unsigned col, unsigned row) {
				ASSERT(col < cols() && row < rows());
				return cells_[row * cols() + col];
			}

			/* Not needed?
			Cell& at(helpers::Point const& p) {
				return at(p.col, p.row);
			}
			*/

			Terminal::Cell const& at(unsigned col, unsigned row) const {
				ASSERT(col < cols() && row < rows());
				return cells_[row * cols() + col];
			}

			/* Not needed?
			Cell const& at(helpers::Point const& p) const {
				return at(p.col, p.row);
			}
			*/

			~Buffer() {
				delete[] cells_;
			}

		private:

			friend class Terminal;

			Buffer(Terminal* terminal) :
				terminal_(terminal),
				cells_(new Cell[cols() * rows()]) {
				ASSERT(terminal != nullptr) << "Buffer must always belong to a terminal";
			}

			/** Resizes the buffer.
			 */
			void resize() {
				// TODO resizing the terminal should preserve some of the original context, if applicable
				delete[] cells_;
				cells_ = new Cell[cols() * rows()];
			}

			/** The terminal to which the buffer belongs.
			 */
			Terminal* terminal_;

			/** Number of columns and rows the buffer stores.
			 */
			unsigned cols_;
			unsigned rows_;

			/** The underlying array of cells.
			 */
			Cell* cells_;
		};

		class Frontend : public helpers::Object {
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


			virtual ~Frontend() {
				detachFromTerminal();
			}

		protected:

			/** Called by the attached terminal backend when new dirty cells exist which must be repainted.
             */
			virtual void repaint() = 0;

			Frontend(unsigned cols, unsigned rows) :
				terminal_(nullptr),
				cols_(cols),
				rows_(rows) {
			}

			virtual void resize(unsigned cols, unsigned rows) {
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

			void doRepaint(RepaintEvent& e) {
				repaint();
			}

			void detachFromTerminal() {
				if (terminal_ == nullptr)
					return;
				terminal_->onRepaint -= HANDLER(Frontend::doRepaint);
				terminal_ = nullptr;
			}

			void attachToTerminal(Terminal* terminal) {
				terminal_ = terminal;
				terminal->resize(cols_, rows_);
				terminal_->onRepaint += HANDLER(Frontend::doRepaint);
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

			void setTerminal(Terminal* terminal) {
				if (terminal_ != terminal) {
					detachFromTerminal();
					if (terminal != nullptr)
						attachToTerminal(terminal);
				}
			}

		protected:

			Backend() :
				terminal_(nullptr) {
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
				ASSERT(terminal->backend_ == nullptr) << "Terminal already attached";
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

			/** Sets the pseudoterminal object associated with the backend. 
			 */
			void setPty(PTY* value);

			~PTYBackend() override {
				delete[] buffer_;
			}

		protected:

			/** When the backend is resized, propagates the resize information to the underlying PTY, if present. 
			 */
			void resize(unsigned cols, unsigned rows) override;


			PTYBackend(size_t bufferSize = 1024) :
				bufferSize_(bufferSize),
				buffer_(new char[bufferSize]),
				writeStart_(buffer_) {
			}

			/** Called when data is received.

				The first argument is the buffer containing the received data. The second argument is the size of available data from the buffer start. The function must return the number of bytes from the input actually processed. The remaining bytes will be prefetched to the next call to the dataReceived when more data has been sent by the client.
			 */
			virtual size_t dataReceived(char * buffer, size_t size) = 0;

			/** Sends given data using the attached PTY. 
			    
				Returns the number of bytes send which should be identical to the second argument (size) unless there was an I/O error while sending. 
			 */
			size_t sendData(char * buffer, size_t size);

			/** Resizes the internal buffer. 
			 */
			void resizeBuffer(size_t newSize);

			/** Using the internal buffer, reads new data from the target process via the attached PTY and then calls the dataReceived() method to process it. 

			    If there are any unprocessed data left after the call, they are copied to the beginning of the buffer so that next read will be appended after them. 
			 */
			void receiveAndProcessData();

			/** Starts a new thread that waits for new data from the attached PTY and when received calls the dataReceived periodically. 
			
			    Any unprocessed data is always prepended to the next received bits. 
			 */
			void threadedReceiver();

		private:
			PTY * pty_;

			size_t bufferSize_;
			char * buffer_;
			char * writeStart_;
		};

		Terminal(unsigned cols, unsigned rows, Backend* backend = nullptr) :
			cols_(cols),
			rows_(rows),
			buffer_(this),
			backend_(nullptr) {
			if (backend) {
				backend->setTerminal(this);
				ASSERT(backend_ == backend) << "Terminal backend not set property when attaching";
			}
		}

		// events

		helpers::Event<RepaintEvent> onRepaint;

		/** Returns the width and height of the terminal.
		 */
		unsigned cols() const {
			return cols_;
		}

		unsigned rows() const {
			return rows_;
		}

		/** Returns the buffer associated with the terminal. 
		 */
		Buffer const& buffer() const {
			return buffer_;
		}

		Buffer& buffer() {
			return buffer_;
		}

		void resize(unsigned cols, unsigned rows) {
			if (cols_ != cols || rows_ != rows) {
				cols_ = cols;
				rows_ = rows;
				buffer_.resize();
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
				backend_->keyDown(k);
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

	private:
		/** Size of the terminal. 
		 */
		unsigned cols_;
		unsigned rows_;

		/** The buffer containing rendering information for all the cells. 
		 */
		Buffer buffer_;

		/** Backend attached to the terminal. 
		 */
		Backend* backend_;

	};


#ifdef HAHA

	/** Virtual Terminal Implementation. 

	    The terminal only implements the storage and programmatic manipulation of the terminal data and does not concern itself with either making the contents visible to the user (this is the job of terminal renderer), or by specifying ways on how & when the contents of the terminal should be updated, which is a job of classes that inherit from the virtual terminal (such as PTYTerminal). 
	 */
	class Terminal : public helpers::Object {
	public:

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

		virtual void mouseMove(unsigned col, unsigned row) = 0;
		virtual void mouseDown(unsigned col, unsigned row, unsigned button) = 0;
		virtual void mouseUp(unsigned col, unsigned row, unsigned button) = 0;
		virtual void mouseWheel(unsigned col, unsigned row, int offset) = 0;

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

		/** Provides access to the terminal's buffer and associated features.
		 */
	class Buffer {
	public:
		/** Returns the terminal associated with the buffer.
		 */
		Terminal* terminal() {
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
		Cell& at(unsigned col, unsigned row) {
			ASSERT(col < cols() && row < rows());
			return terminal_->cells_[row * cols() + col];
		}

		Cell& at(helpers::Point const& p) {
			return at(p.col, p.row);
		}

		Cell const& at(unsigned col, unsigned row) const {
			ASSERT(col < cols() && row < rows());
			return terminal_->cells_[row * cols() + col];
		}

		Cell const& at(helpers::Point const& p) const {
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
		Terminal* terminal_;
	}; // Terminal::Buffer




#endif

} // namespace vterm

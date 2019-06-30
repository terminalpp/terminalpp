#pragma once

#include <thread>
#include <atomic>
#include <mutex>
#include <string>
#include <list>

#include "helpers/object.h"
#include "helpers/shapes.h"
#include "helpers/time.h"
#include "helpers/log.h"

#include "color.h"
#include "font.h"
#include "char.h"
#include "key.h"
#include "mouse.h"
#include "pty.h"


namespace vterm {

	class PTY;

	// TODO this should go to UI when we have UI
	class Selection {
	public:
		helpers::Point start;
		helpers::Point end;

		Selection() = default;

		Selection(helpers::Point const& start, helpers::Point const& end) :
			start(start),
			end(end) {
		}

		Selection(unsigned left, unsigned top, unsigned right, unsigned bottom) :
			start(left, top),
			end(right, bottom) {
		}

		bool empty() const {
			return start == end;
		}

		bool contains(unsigned col, unsigned row) const {
			// if the row is outside the selectiomn boundaries, return false quickly
			if (row < start.row || row >= end.row)
				return false;
			// if its the first row, check if it is after start of the selection
			if (row == start.row) {
				// or if the selection is single row after start and before end
				if (end.row == start.row + 1)
					return col >= start.col && col < end.col;
				else
					return col >= start.col;
			}
			// if it is last row, then if col is before end
			if (row == end.row - 1)
				return col < end.col;
			// otherwise return true, one of the full rows
			return true;
		}

	};



	/** Implementation of the terminal. 

	    The terminal provides encapsulation over the screen buffer and provides the communication between the frontend and backend. The terminal frontend is responsible for rendering the contents of the terminal to the user and sending the terminal the user input events. The backend of the terminal relays the user input events to the underlying process and reads from the process updates to the terminal state and stores them. 
	 */
	class Terminal : public helpers::Object {
	public:


		typedef helpers::EventPayload<void, helpers::Object> RepaintEvent;
		typedef helpers::EventPayload<std::string, helpers::Object> TitleChangeEvent;
		typedef helpers::EventPayload<std::string, helpers::Object> ClipboardUpdateEvent;
		typedef helpers::EventPayload<void, helpers::Object> NotificationEvent;

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
			Font font() const {
				return font_;
			}

			void setFont(Font const& value) {
				font_ = value;
				dirty_ = true;
			}

			/** Text color.
			 */
			Color fg() const {
				return fg_;
			}

			void setFg(Color value) {
				fg_ = value;
				dirty_ = true;
			}

			/** Background color.
			 */
			Color bg() const {
				return bg_;
			}

			void setBg(Color value) {
				bg_ = value;
				dirty_ = true;
			}

			/** Character to be displayed (utf8).
			 */
			Char::UTF8 c() const {
				return c_;
			}

			void setC(Char::UTF8 value) {
				c_ = value;
				dirty_ = true;
				lineEnd_ = false;
			}

			/** Determines if the cell is dirty, i.e. it should be redrawn.
			 */
			bool dirty() const {
				return dirty_;
			}

			void markDirty(bool value = true) {
				dirty_ = value;
			}

			bool isLineEnd() const {
				return lineEnd_;
			}

			void markAsLineEnd(bool value = true) {
				lineEnd_ = value;
			}

			/** Default constructor for a cell created white space on a black background.
			 */
			Cell() :
				fg_(Color::White()),
				bg_(Color::Black()),
				c_(' '),
				dirty_(0),
			    lineEnd_(0) {
			}

			Cell& operator = (Cell const& other) {
				if (this != &other) {
					memcpy(this, &other, sizeof(Cell));
					dirty_ = true;
				}
				return *this;
			}

		private:
			Font font_;
			Color fg_;
			Color bg_;
			Char::UTF8 c_;
			struct {
				// when dirty, the cell should be redrawn
				unsigned dirty_ : 1;
				// indicates last character of a line
				unsigned lineEnd_ : 1;
			};
		};

		class Screen {
		public:

			Screen(unsigned cols, unsigned rows) :
				cols_(cols),
				rows_(rows),
				cells_(new Cell[cols * rows]) {
			}

			Screen(Screen const& from) :
				cols_(from.cols_),
				rows_(from.rows_),
				cursor_(from.cursor_),
				cells_(new Cell[from.cols_ * from.rows_]) {
				memcpy(cells_, from.cells_, sizeof(Cell) * cols_ * rows_);
			}

			Screen& operator = (Screen const& other) {
				if (cols_ != other.cols_ || rows_ != other.rows_) {
					cols_ = other.cols_;
					rows_ = other.rows_;
					delete[] cells_;
					cells_ = new Cell[cols_ * rows_];
				}
				cursor_ = other.cursor_;
				memcpy(cells_, other.cells_, sizeof(Cell) * cols_ * rows_);
				return *this;
			}

			~Screen() {
				delete[] cells_;
			}

			unsigned cols() const {
				return cols_;
			}

			unsigned rows() const {
				return rows_;
			}

			Cursor const& cursor() const {
				return cursor_;
			}

			Cursor & cursor() {
				return cursor_;
			}

			Cell const& at(unsigned col, unsigned row) const {
				ASSERT(col < cols_ && row < rows_) << "Cell out of range";
				return cells_[row * cols_ + col];
			}

			Cell & at(unsigned col, unsigned row) {
				ASSERT(col < cols_ && row < rows_) << "Cell out of range";
				return cells_[row * cols_ + col];
			}

			/** Resizes the screen to given number of columns and rows. 

			    TODO The resize resets cursor to the top left corner. This is probably wrong and should be done differently
			 */
			void resize(unsigned cols, unsigned rows) {
				if (cols_ != cols || rows_ != rows) {
					// resize the actual cells
					resizeCells(cols, rows);
					// update the size
					cols_ = cols;
					rows_ = rows;
				}
			}

			/** Marks the entire screen dirty. 
			 */
			void markDirty() {
				for (size_t i = 0, e = static_cast<size_t>(cols_) * rows_; i < e; ++i)
					cells_[i].markDirty(true);
			}

		private:

			friend class Terminal;

			/** Resizes the cell buffer. 

			    First creates the new buffer and clears it. Then we determine the latest line to be copied (since the client app is supposed to rewrite the current line). We also calculate the offset of this line to the current cursor line, which is important if the last line spans multiple terminal lines since we must adjust the cursor position accordingly then. 

				The line is the copied. 
			 */
			void resizeCells(unsigned newCols, unsigned newRows) {
				unsigned x = cursor_.row * cols_ + cursor_.col + 1;
				unsigned stopRow = 0;
				while (x-- > 0) {
					if (cells_[x].isLineEnd()) {
						// update new col & row
						stopRow = (x / cols_) + 1;
						break;
					}
				}
				// copy the contents
				unsigned rowOffset = cursor_.row - stopRow;
				cursor_.col = 0;
				cursor_.row = 0;
				Cell* newCells = new Cell[newCols * newRows];
				for (unsigned y = 0; y < rows_; ++y) {
					for (unsigned x = 0; x < cols_; ++x) {
						if (y == stopRow) {
							delete[] cells_;
							cells_ = newCells;
							cursor_.row += rowOffset;
							return;
						}
						Cell& cell = at(x, y);
						newCells[cursor_.row * newCols + cursor_.col] = cell;
						// if the cell is new line, or if moving to next character would be a new line, increase cursor row
						if (cell.isLineEnd() || (++cursor_.col == newCols)) {
							++cursor_.row;
							cursor_.col = 0;
						}
						// if we are past the new buffer, move all up
						if (cursor_.row == newRows) {
							// scroll up
							memmove(
								newCells,
								newCells + newCols,
								(sizeof(Cell) * newCols) * (newRows - 1));
							--cursor_.row;
							// clear the line
							for (unsigned c = 0; c < newCols; ++c)
								newCells[cursor_.row * newCols + c] = Cell();
						}
						// if we hit new line in the old buffer, skip the rest of the line
						if (cell.isLineEnd())
							break;
					}
				}
			}

			unsigned cols_;
			unsigned rows_;
			Cursor cursor_;
			Cell* cells_;
		};

		/** 

		    TODO This is perhaps very stupid name. 
		 */
		class ScreenLock {
		public:

			ScreenLock(ScreenLock&& from) noexcept :
				terminal_(from.terminal_) {
				from.terminal_ = nullptr;
			}

			~ScreenLock() {
				if (terminal_ != nullptr)
					terminal_->releaseScreenLock();
			}

			/** Returns the copy of the screen. 
			 */
			void getScreenCopy(Screen & into) const {
				ASSERT(terminal_ != nullptr);
				into = terminal_->screen_;
			}

			Screen const& operator * () const {
				ASSERT(terminal_ != nullptr);
				return terminal_->screen_;
			}

			Screen& operator * () {
				ASSERT(terminal_ != nullptr);
				return terminal_->screen_;
			}

			Screen const* operator -> () const {
				ASSERT(terminal_ != nullptr);
				return & terminal_->screen_;
			}

			Screen * operator -> () {
				ASSERT(terminal_ != nullptr);
				return &terminal_->screen_;
			}

		private:

			friend class Terminal;

			ScreenLock(Terminal* terminal, bool priorityRequest) :
				terminal_(terminal) {
				terminal_->acquireScreenLock(priorityRequest);
			}


			Terminal* terminal_;
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

			virtual void clipboardUpdated(ClipboardUpdateEvent& e) = 0;

			Renderer(unsigned cols, unsigned rows) :
				terminal_(nullptr),
				cols_(cols),
				rows_(rows) {
			}

			virtual void resize(unsigned cols, unsigned rows) {
				cols_ = cols;
				rows_ = rows;
				if (terminal_)
					terminal_->lockScreen()->resize(cols, rows);
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
				terminal_->onClipboardUpdated -= HANDLER(Renderer::clipboardUpdated);
				terminal_ = nullptr;
			}

			virtual void attachToTerminal(Terminal* terminal) {
				terminal_ = terminal;
				terminal_->onRepaint += HANDLER(Renderer::repaint);
				terminal_->onTitleChange += HANDLER(Renderer::titleChange);
				terminal_->onClipboardUpdated += HANDLER(Renderer::clipboardUpdated);
				terminal_->lockScreen()->resize(cols_, rows_);
			}

			Terminal* terminal_;

			unsigned cols_;
			unsigned rows_;

		};

		Terminal(unsigned cols, unsigned rows) :
			priorityRequests_(0),
			oldCols_(0),
			oldRows_(0),
			screen_(cols, rows) {
		}

		~Terminal() override {
		}

		// events

		// TODO rename to onClipboardUpdate

		helpers::Event<RepaintEvent> onRepaint;
		helpers::Event<TitleChangeEvent> onTitleChange;
		helpers::Event<ClipboardUpdateEvent> onClipboardUpdated;

		/** Triggered when backend wants to notify the user. 

		    For now, only BEL character terminal notification is supported, but in the future multiple notification types might be possible. 
		 */
		helpers::Event<NotificationEvent> onNotification;

		// TODO : or should the next 3 functions be abstract? 

		/** Returns true if the terminal is interested in mouse events.
		 */
		virtual bool captureMouse() const {
			return true;
		}

		/** Returns the default background color of the terminal.
		 */
		virtual vterm::Color defaultBackgroundColor() const {
			return vterm::Color::Black();
		}

		/** Returns the default foreground color of the terminal.
		 */
		virtual vterm::Color defaultForegroundColor() const {
			return vterm::Color::White();
		}

		/** Acquires the lock on the screen via which the screen contents can be read or updated. 

		    If there is a priority request pending, then non-priority requests which did not acquire the lock yet are required to yield so that the priority request can be satisfied as soon as possible. 
		 */
		ScreenLock lockScreen(bool priorityRequest = false) {
			return ScreenLock(this, priorityRequest);
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

		std::string getText(Selection const& selection) const;

		// keyboard events

		virtual void keyDown(Key k) = 0;

		virtual void keyUp(Key k) = 0;

		virtual void keyChar(Char::UTF8 c) = 0;

		// mouse events

		virtual void mouseDown(unsigned col, unsigned row, MouseButton button) = 0;

		virtual void mouseUp(unsigned col, unsigned row, MouseButton button) = 0;

		virtual void mouseWheel(unsigned col, unsigned row, int by) = 0;

		virtual void mouseMove(unsigned col, unsigned row) = 0;

		// clipboard events

		virtual void paste(std::string const& what) = 0;

	protected:

		/** Invoked by the terminal after screen lock release if the lock resized the terminal. 
		 */
		virtual void doOnResize(unsigned cols, unsigned rows) = 0;

		/** Acquires the screen lock 
		 */
		void acquireScreenLock(bool priorityRequest) {
			if (priorityRequest) {
				++priorityRequests_;
				m_.lock();
				--priorityRequests_;
			} else {
				while (true) {
					while (priorityRequests_ > 0)
						std::this_thread::yield();
					m_.lock();
					if (priorityRequests_ > 0) {
						m_.unlock();
						continue;
					}
					break;
				}
			}
			// we are locked now, store the 
			oldCols_ = screen_.cols_;
			oldRows_ = screen_.rows_;
		}

		/** Releases the screen lock. 

		    Checks whether during the lock the screen was resized and if so notifies the backend. 
		 */
		void releaseScreenLock() {
			bool resized = (oldCols_ != screen_.cols_) || (oldRows_ != screen_.rows_);
			oldCols_ = screen_.cols_;
			oldRows_ = screen_.rows_;
			m_.unlock();
			// make sure that we are calling the event *after* the lock is released
			if (resized)
				doOnResize(oldCols_, oldRows_);
		}

		/** Guard for accessing the terminal buffer. 
		 */
		std::mutex m_;

		/** Determines whether there is a priority request for the lock. 
		 */
		std::atomic<unsigned> priorityRequests_;

		/** Backup cols and rows to detect resize event when screen lock is released. 
		 */
		unsigned oldCols_;
		unsigned oldRows_;
		
		/** The actual screen and its properties. 
		 */
		Screen screen_;

		/** Title of the terminal. 
		 */
		std::string title_;

	}; // vterm::Terminal

	class PTYTerminal : public Terminal {
	public:
		PTYTerminal(unsigned cols, unsigned rows, PTY* pty, size_t bufferSize) :
			Terminal(cols, rows),
			pty_(pty),
		    bufferSize_(bufferSize),
		    buffer_(new char[bufferSize]),
		    writeStart_(buffer_) {
			readerThread_ = std::thread([this]() {
				while (true) {
					size_t read = pty_->read(writeStart_, bufferSize_ - (writeStart_ - buffer_));
					// if 0 bytes were read, terminate the thread
					if (read == 0)
						break;
					// otherwise add any pending data from previous cycle
					read += (writeStart_ - buffer_);
					size_t processed = doProcessInput(buffer_, read);
					// if not everything was processed, copy the unprocessed part at the beginning and set writeStart_ accordingly
					if (processed != read) {
						memcpy(buffer_, buffer_ + processed, read - processed);
						writeStart_ = buffer_ + read - processed;
					} else {
						writeStart_ = buffer_;
					}
				}
			});
		}

		~PTYTerminal() override {
			pty_->terminate();
			readerThread_.join();
			delete pty_;
			delete [] buffer_;
		}

		PTY const& pty() const {
			return *pty_;
		}

		PTY & pty() {
			return *pty_;
		}

	protected:

		void ptyWrite(char const* bytes, size_t size) {
			size_t sent = pty_->write(bytes, size);
			// TODO some better error
			ASSERT(sent == size);
		}

		/** When the terminal is resized, calls the PTY's resize method so that the client is notified of the size change. 
		 */
		void doOnResize(unsigned cols, unsigned rows) override {
			pty_->resize(cols, rows);
		}

		virtual size_t doProcessInput(char * buffer, size_t size) = 0;

	private:

		PTY * pty_;
		size_t bufferSize_;
		char * buffer_;
		char * writeStart_;
		std::thread readerThread_;

	}; //vterm::PTYTerminal

} // namespace vterm

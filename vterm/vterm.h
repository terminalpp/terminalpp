#pragma once

#include <mutex>
#include <string>
#include <list>

#include "helpers/object.h"

#include "color.h"
#include "font.h"
#include "char.h"
#include "key.h"

namespace vterm {

	/** Dimension of the terminal in number of columns and rows it can display. 
	 */
	struct Dimension {
		unsigned cols;
		unsigned rows;
	}; // vterm::Dimension

	/** Rectangle definition. 

	    Rectangle is defined with it top left corner coordinates and width and height (columns and rows). Functions to calculate bottom right coordinates are provided as well. 
	 */
	struct Rect {
		unsigned left;
		unsigned top;
		unsigned cols;
		unsigned rows;

		unsigned right() const {
			return left + cols;
		}

		unsigned bottom() const {
			return top + rows;
		}
	}; // vterm::Rect

	typedef helpers::EventPayload<Dimension, helpers::Object> TerminalResizeEvent;
	typedef helpers::EventPayload<Rect, helpers::Object> TerminalRepaintEvent;

	/** The virtual terminal. 

	    The virtual terminal encapsulates the screen buffer and required communications.

	 */
	class VTerm: public helpers::Object {
	public:

		// classes

		/** Describes a single character cell of the terminal. 
		 */
		class Cell; 

		/** Provides access to the contents of the terminal. 
		 */
		class Screen; 

		/** Base class for any objects whose output is to be displayed in the terminal. 
		 */
		class Process; 

		/** Base class for any renderer of a virtual terminal. 
		 */
		class Renderer;

		// events

		/** Triggered when the terminal size changes. 
		 */
		helpers::Event<TerminalResizeEvent> onResize;

		/** Triggered when portion of the terminal screen has changed and needs to be redrawn. 
		 */
		helpers::Event<TerminalRepaintEvent> onRepaint;

		// constructors & destructors

		/** Creates the new terminal.
         */
		VTerm(unsigned cols, unsigned rows, Process * process = nullptr);

		~VTerm() override;

		// properties

		/** Returns the size of the terminal. 
		 */
		Dimension size() const;

		/** Returns the process associated with the terminal. 
		 */
		Process * process() const {
			return process_;
		}

		/** Sets the process associated with the terminal. 
		 */
		void setProcess(Process * process);

		// methods

		/** Creates a Screen object so that the contents of the terminal can be accessed.
		 */
		Screen getScreen();
		Screen const getScreen() const;

		/** Resizes the terminal.
		 */
		void resize(unsigned cols, unsigned rows);

	protected:

		/** Processes input from the attached process. 

		    The method is given the beginning of the buffer and size of valid data. The size can be modified to reflect that not all of the available data has been processed correctly in case of only partial sequence has been read from the attached process so far. 
		 */
		virtual void processInput(char * buffer, size_t & size) = 0;

		/** The actual contents of the terminal. 
		 */
		Cell * cells_;

		/** Number of columns. 
		 */
		unsigned cols_;

		/** Number of rows. 
		 */
		unsigned rows_;

		/** Guard to make sure there is only one buffer materialized at any given time. 
		 */
		std::mutex m_;

		Process * process_;

	private:

		friend class Process;

		/** Size of the input process buffer in bytes. 

		    TODO perhaps this should be user modifiable for things like file transfers, etc. 
		 */
		static constexpr size_t BUFFER_SIZE = 512;

		/** Attaches the terminal to given process.
		 */
		void attachProcess(Process * process);

		void detachProcess();

	};

	/** Contains information about the font, colors, effects and character to be displayed.
     */
	class VTerm::Cell {
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

		/** Default constructor for a cell created white space on a black background.
		 */
		Cell() :
			fg(Color::White),
			bg(Color::Black),
			c(' ') {
		}
	};


	/** A Screen instance is the only way to access the buffer of the terminal and read or change it.
	
	    For a given terminal can be at most one Screen object at any given time, which is enforced by a per terminal instance lock. This is to make sure that the terminal works well in multi-threaded settings where different threads may be responsible for populating and displaying the contents of the terminal to limit any data races. 
	 */
	class VTerm::Screen {
	public:

		Screen(Screen const &) = delete;

		Screen(Screen && from) :
			terminal_(from.terminal_) {
			ASSERT(terminal_ != nullptr);
			from.terminal_ = nullptr;
		}

		~Screen() {
			if (terminal_ != nullptr)
				terminal_->m_.unlock();
		}

		Screen & operator = (Screen const &) = delete;

		/** Returns the terminal the buffer is associated with. 
		 */
		VTerm * terminal() const {
			return terminal_;
		}

		/** Returns number of columns the underlying terminal can store/display. 
		 */
		unsigned cols() const {
			return terminal_->cols_;
		}

		/** Returns number of rows the underlying terminal can store/display. 
		 */
		unsigned rows() const {
			return terminal_->rows_;
		}

		Cell const & at(unsigned col, unsigned row) const {
			ASSERT(col < cols() && row < rows());
			return terminal_->cells_[row * cols() + col];
		}

		Cell & at(unsigned col, unsigned row) {
			ASSERT(col < cols() && row < rows());
			return terminal_->cells_[row * cols() + col];
		}

	private:
		friend class VTerm;

		Screen(VTerm * terminal) :
			terminal_(terminal) {
			terminal_->m_.lock();
		}

		VTerm * terminal_;

	};

	class VTerm::Process {
	public:

		VTerm * terminal() const {
			return terminal_;
		}

		/** Called by the terminal when it gets resized.
		 */
		virtual void resize(unsigned cols, unsigned rows) = 0;

		// TODO add key presses, etc. 

	protected:
		static constexpr size_t DEFAULT_BUFFER_SIZE = 512;

		Process(size_t bufferSize = DEFAULT_BUFFER_SIZE);

		/** Obtains a continus area of the buffer that the attached process can write to.

			NOTE: It is assumed that same thread is responsible for calling both getBuffer and commitBuffer methods.
		 */
		char * getInputBuffer(size_t & size) {
			size = bufferEnd_ - writeStart_;
			return writeStart_;
		}

		/** When the attached process writes the received data to the obtained buffer, the commit buffer method makes sure the terminal will process the data accordingly.

		 */
		void commitInputBuffer(char * buffer, size_t size);

		VTerm * terminal_;

	private:
		friend class VTerm;

		char * bufferBegin_;
		char * readStart_;
		char * writeStart_;
		char * bufferEnd_;;

	};

	class VTerm::Renderer : public helpers::Object {
	public:

		VTerm * terminal() const {
			return terminal_;
		}

		void setTerminal(VTerm * terminal) {
			if (terminal == terminal_)
				return;
			detachTerminal();
			attachTerminal(terminal);
		}

		unsigned cols() const {
			return cols_;
		}

		unsigned rows() const {
			return rows_;
		}

	protected:

		virtual void repaint(TerminalRepaintEvent & e) = 0;

		/** Resizes the terminal to given size.

			Does not trigger repaint method immediately, which is what the underlying terminal does when it resizes itself.
		 */
		void resizeTerminal(unsigned cols, unsigned rows) {
			cols_ = cols;
			rows_ = rows;
			if (terminal_ != nullptr)
				terminal_->resize(cols, rows);
		}


	private:

		void attachTerminal(VTerm * terminal) {
			ASSERT(terminal_ == nullptr);
			terminal_ = terminal;
			terminal_->onRepaint += HANDLER(Renderer::repaint);
			terminal_->resize(cols_, rows_);
		}

		void detachTerminal() {
			if (terminal_ == nullptr)
				return;
			terminal_->onRepaint -= HANDLER(Renderer::repaint);
			terminal_ = nullptr;
		}

		VTerm * terminal_;
		unsigned cols_;
		unsigned rows_;

	};

} // namespace vterm

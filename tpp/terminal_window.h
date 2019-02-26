#pragma once
#include "application.h"

namespace tpp {

	/** Renderer independent behavior of the terminal window. */
	class TerminalWindow : public helpers::Object {
	public:

		/** Returns the width of the terminal window (in pixels). 
		 */
		unsigned width() const {
			return width_;
		}

		/** Returns the height of the terminal window (in pixels). 
		 */
		unsigned height() const {
			return height_;
		}

		/** Returns the number of columns the terminal window can display. 
		 */
		unsigned cols() const {
			return width_ / fontWidth_;
		}

		/** Returns the number of rows the terminal window can display. 
		 */
		unsigned rows() const {
			return height_ / fontHeight_;
		}

		/** Returns the virtual terminal which is being rendered. 
		 */
		vterm::VirtualTerminal const * terminal() const {
			return terminal_;
		}

		/** Sets the virtual terminal being rendered. 
		 */
		void setTerminal(vterm::VirtualTerminal * terminal) {
			if (terminal_ == terminal)
				return;
			if (terminal_ != nullptr)
				detachTerminal();
			attachTerminal(terminal);
		}

		virtual void show() = 0;
		virtual void hide() = 0;


		/** Destrys the terminal window and detaches the associated terminal. 
		 */
		~TerminalWindow() override {
			if (terminal_ != nullptr)
				detachTerminal();
			// TODO also remove from the list of windows.
		}

	protected:

		TerminalWindow() :
			terminal_(nullptr) {
		}

		/** Triggered when the attached virtual terminal contents should be repainted. 
		 */
		virtual void repaintTerminal(vterm::RepaintEvent & e) = 0;

		/** Resizes the window and the attached terminal. 
		
		    Triggers the repaint of the terminal if the terminal size does not change (since it is assumed the buffer size did change), if the terminal size changes, then it is assumed the terminal will trigger the repaint event itself when it resizes.
		 */
		virtual void resize(unsigned width, unsigned height) {
			width_ = width;
			height_ = height;
			if (terminal_ != nullptr) {
				unsigned c = cols();
				unsigned r = rows();
				if (terminal_->cols() != c || terminal_->rows() != r)
					terminal_->resize(cols(), rows());
/*				else
					repaintTerminal(vterm::RepaintEvent(this, vterm::TerminalRepaint{ 0,0, c, r }));
					*/
			}
		}

		/** Attaches given terminal to the window. 
		 */
		virtual void attachTerminal(vterm::VirtualTerminal * terminal) {
			if (terminal != nullptr) {
				terminal->onRepaint += HANDLER(TerminalWindow::repaintTerminal);
				terminal_ = terminal;
				terminal_->resize(cols(), rows());
			}
			repaintTerminal(vterm::RepaintEvent(this, vterm::TerminalRepaint{ 0, 0, cols(), rows() }));
		}

		/** Detaches the window from the current terminal and sets it to nullptr. 

		    Removes the onPaint handler for the current window from the terminal. 
		 */
		virtual void detachTerminal() {
			terminal_->onRepaint -= HANDLER(TerminalWindow::repaintTerminal);
			terminal_ = nullptr;
		}

		unsigned width_;
		unsigned height_;
		unsigned fontWidth_;
		unsigned fontHeight_;
		vterm::VirtualTerminal * terminal_;
	};


} // namespace tpp
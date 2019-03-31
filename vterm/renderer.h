#pragma once

#include "helpers/object.h"

#include "terminal.h"

namespace vterm {

	/** VTerm Renderer. 

	    The renderer is responsible for displaying on terminal's screen contents, controlling the terminal's behavior (such as its size) and providing it with the inputs (mouse, keyboard, etc.). 

		A common renderer implementation is a graphical window which displays the contents and provides mouse & keyboard input, but other renderers are possible as well (such as simple ANSI sequence encoder so that a virtual terminal can be embedded inside GUI applications. 
	 */
	class Renderer : public helpers::Object {
	public:

		/** Returns the terminal object the renderer is displaying, or nullptr if no terminal is attached. 
		 */
		Terminal * terminal() const {
			return terminal_;
		}

		/** Attaches the renderer to given terminal. 
		 */
		void attachTerminal(Terminal * terminal) {
			if (terminal == terminal_)
				return;
			doAttachTerminal(terminal);
		}

		void detachTerminal() {
			if (terminal_ == nullptr)
				return;
			doDetachTerminal(terminal_);
		}

		/** Returns the number of columns the renderer is capable of displaying. 
		 */
		unsigned cols() const {
			return cols_;
		}

		/** Returns the number of rows the renderer is capable of displaying. 
		 */
		unsigned rows() const {
			return rows_;
		}

		/** Detaches from the terminal when deleted.
		 */
		~Renderer() {
			if (terminal_ != nullptr)
				detachTerminal();
		}

	protected:

		Renderer(unsigned cols, unsigned rows) :
			terminal_(nullptr),
		    cols_(cols), 
		    rows_(rows) {
		}

		/** Triggered when the selected part of the terminal screen has been changed. 

		    This method is to be overriden by the respective renderers. 
		 */
		virtual void repaint(Terminal::RepaintEvent & e) = 0;

		/** Triggers repainting of the entire terminal contents. 
		 */
		void repaintAll() {
			repaint(Terminal::RepaintEvent(nullptr, helpers::Rect{ 0,0,cols_,rows_ }));
		}

		/** Resizes the terminal to given size.

			Does not trigger repaint method immediately, which is what the underlying terminal does when it resizes itself.
		 */
		void resize(unsigned cols, unsigned rows) {
			cols_ = cols;
			rows_ = rows;
			if (terminal_ != nullptr)
				terminal_->resize(cols, rows);
		}

		virtual void doAttachTerminal(Terminal * terminal) {
			if (terminal_ != nullptr)
				doDetachTerminal(terminal_);
			// set the terminal 
			terminal_ = terminal;
			// inform the terminal that a new renderer has been attached and subscribe to its repaint event
			if (terminal_ != nullptr) {
				terminal_->onRepaint += HANDLER(Renderer::repaint);
				if (!terminal_->resize(cols_, rows_))
					repaintAll();
			}
		}

		virtual void doDetachTerminal(Terminal * terminal) {
			ASSERT(terminal_ != nullptr);
			terminal_->onRepaint -= HANDLER(Renderer::repaint);
			terminal_ = nullptr;
		}


	private:

		Terminal * terminal_;
		unsigned cols_;
		unsigned rows_;

	};

}
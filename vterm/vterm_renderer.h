#pragma once

#include "virtual_terminal.h"

namespace vterm {

	class VirtualTerminal::Renderer {
	public:
		virtual ~Renderer() {
			if (terminal_ != nullptr) {
				ASSERT(terminal_->renderer_ == this) << "Corrupted renderer-terminal relation";
				terminal_->detachRenderer();
			}
		}

		/** Returns the number of columns the renderer displays. 
		 */
		unsigned cols() const {
			return cols_;
		}

		/** Returns the number of rows the renderer displays. 
		 */
		unsigned rows() const {
			return rows_;
		}

		/** Returns the terminal associated with the renderer. 

		    Returns nullptr if the renderer is not associated with any terminal. 
		 */
		VirtualTerminal * terminal() {
			return terminal_;
		}

		/** Sets the terminal associated with the renderer. 
		 */
		void setTerminal(VirtualTerminal * terminal) {
			if (terminal_ == terminal)
				return;
			if (terminal_ != nullptr)
				terminal_->detachRenderer();
			if (terminal != nullptr) {
				terminal_ = terminal;
				terminal_->attachRenderer(this);
			}
			repaint(0, 0, cols_, rows_);
		}
	protected:
		friend class VirtualTerminal;

		Renderer() :
			terminal_(nullptr),
			cols_(0),
			rows_(0) {
		}

		/** Resizes the terminal to given size. 

		    Does not trigger repaint method immediately, which is what the underlying terminal does when it resizes itself. 
		 */
		void resizeTerminal(unsigned cols, unsigned rows) {
			cols_ = cols;
			rows_ = rows;
			if (terminal_ != nullptr) 
				terminal_->resize(cols, rows);
		}

		/** Sends given key event to the terminal. 
		 */
		virtual void keyEvent() {

		}

		/** Sends given mouse event to the terminal. 
		 */
		virtual void mouseEvent() {

		}

		/** Redraws the appropriate rectangle of the terminal. 
		 */
		virtual void repaint(unsigned left, unsigned top, unsigned cols, unsigned rows) = 0;

	private:

		/** The associated terminal. 
		 */
		VirtualTerminal * terminal_;

		/** Number of columns the renderer can display. 
		 */
		unsigned cols_;
		
		/** Number of rows the renderer can display. 
		 */
		unsigned rows_;

	}; // vterm::VirtualTerminal::Renderer


} // namespace vterm
#include "virtual_terminal.h"

namespace vterm {

	void VirtualTerminal::resize(unsigned cols, unsigned rows) {
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
		// if we have renderer attached, call its paint
		if (renderer_ != nullptr)
			renderer_->repaint(0, 0, cols_, rows_);
		// trigger the onResize event
		trigger(onResize, TerminalSize{ cols, rows });
		trigger(onRepaint, TerminalRepaint{ 0,0,cols, rows });
	}


	void VirtualTerminal::detachRenderer() {
		ASSERT(renderer_ != nullptr) << "Cannot detach from null renderer";
		renderer_ = nullptr;
	}

	void VirtualTerminal::attachRenderer(Renderer * renderer) {
		ASSERT(renderer_ == nullptr) << "Renderer already attached";
		ASSERT(renderer != nullptr) << "Null renderer cannot be attached";
		// set the renderer
		renderer_ = renderer;
		// update the terminal size
		resize(renderer->cols(), renderer->rows());
	}


} // namespace vterm
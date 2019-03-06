#include "gdi_application.h"

#include "gdi_terminal_window.h"



namespace tpp {

	using namespace vterm;


	std::unordered_map<vterm::Font, GDIFont *> GDIFont::Fonts_;


	unsigned GDITerminalWindow::BorderWidth_ = 0;
	unsigned GDITerminalWindow::BorderHeight_ = 0;


	std::unordered_map<HWND, GDITerminalWindow *> GDITerminalWindow::Windows_;



	/** When the window is resized, the shadow buffer is deleted and then the resize method of the GUIRenderer is called. The effect of this is that the size change will be propagated to the terminal, which will resize itself and then ping the renderer to repaint, at which stage the WM_PAINT windows event will be received and the handler will create proper shadow buffer and call the actual repainter. 
	 */
	void GDITerminalWindow::resize(unsigned width, unsigned height) {
		if (width_ == width && height_ == height)
			return;
		if (buffer_ != nullptr) {
			DeleteObject(buffer_);
			buffer_ = nullptr;
		}
		TerminalWindow::resize(width, height);
	}

	/** Updates the respective section of the shadow buffer by loading the state from the screen buffer of the associated terminal. 

	    If shadow buffer is not valid, repaint does nothing and instead waits for the WM_PAINT event to recreate the shadow buffer and then call it when really needed. This also helps with having the shadow buffer properties exactly match the properties of the HDC used to draw the window. 
	 */

	void GDITerminalWindow::repaint(unsigned left, unsigned top, unsigned cols, unsigned rows) {
		// if the buffer is not valid, don't bother repainting, the WM_PAINT will call it when ready
		if (buffer_ == nullptr)
			return;
		paintShadowBuffer(left, top, cols, rows);
		PostMessage(hWnd_, WM_PAINT, 0, 0);
	}

	void GDITerminalWindow::paintShadowBuffer(unsigned left, unsigned top, unsigned cols, unsigned rows) {
		vterm::VirtualTerminal::ScreenBuffer sb(terminal()->screenBuffer());
		// if the screen buffer is empty, there is nothing to paint, exit
		if (sb.cols() == 0 || sb.rows() == 0)
			return;
		vterm::VirtualTerminal::ScreenCell const & firstCell = sb.at(0, 0);
		Color lastFg = firstCell.fg;
		Color lastBg = firstCell.bg;
		vterm::Font lastFont = firstCell.font;
		SetTextColor(memoryBuffer_, RGB(lastFg.red, lastFg.green, lastFg.blue));
		SetBkColor(memoryBuffer_, RGB(lastBg.red, lastBg.green, lastBg.blue));
		SelectObject(memoryBuffer_, GDIFont::GetOrCreate(memoryBuffer_, lastFont)->handle());
		for (unsigned r = top, re = top + rows; r != re; ++r) {
			for (unsigned c = left, ce = left + cols; c != ce; ++c) {
				// get the cell info
				vterm::VirtualTerminal::ScreenCell const & cell = sb.at(c, r);
				// update rendering properties if necessary
				if (cell.fg != lastFg) {
					lastFg = cell.fg;
					SetTextColor(memoryBuffer_, RGB(lastFg.red, lastFg.green, lastFg.blue));
				}
				if (cell.bg != lastBg) {
					lastBg = cell.bg;
					SetBkColor(memoryBuffer_, RGB(lastBg.red, lastBg.green, lastBg.blue));
				}
				// TODO this will not work for blinking text
				if (cell.font != lastFont) {
					lastFont = cell.font;
					SelectObject(memoryBuffer_, GDIFont::GetOrCreate(memoryBuffer_, lastFont)->handle());
				}
				// draw the cell contents
				// ISSUE 4 - make sure we can print full Unicode, not just UCS2
				wchar_t wc = cell.c.toWChar();
				TextOutW(memoryBuffer_, c * fontWidth_, r * Settings.fontHeight, &wc, 1);
			}
		}

	}

	LRESULT CALLBACK GDITerminalWindow::EventHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
		auto i = Windows_.find(hWnd);
		GDITerminalWindow * tw = (i == Windows_.end()) ? nullptr : i->second;
		switch (msg) {
			/* When the window is created, the border width and height of a terminal window is determined and the window's size is updated to adjust for it. */
		case WM_CREATE: {
			CREATESTRUCT & cs = *reinterpret_cast<CREATESTRUCT*>(lParam);
			RECT r;
			r.left = cs.x;
			r.right = cs.x + cs.cx;
			r.top = cs.y;
			r.bottom = cs.y + cs.cy;
			AdjustWindowRectEx(&r, cs.style, false, cs.dwExStyle);
			unsigned bw = r.right - r.left - cs.cx;
			unsigned bh = r.bottom - r.top - cs.cy;
			if (bw != 0 || bh != 0) {
				BorderWidth_ = bw;
				BorderHeight_ = bh;
				SetWindowPos(hWnd, HWND_TOP, cs.x, cs.y, cs.cx + bw, cs.cy + bh, SWP_NOZORDER);
			}
			break;
		}
		case WM_CLOSE:
			DestroyWindow(hWnd);
			break;
		case WM_DESTROY:
			ASSERT(tw != nullptr) << "Attempt to destroy unknown window";
			Windows_.erase(i);
			if (Windows_.empty())
				PostQuitMessage(0);
			break;
			/* Called when the window is resized interactively by the user. Makes sure that the window size snaps to discrete terminal sizes. */
		case WM_SIZING: {
			RECT * winRect = reinterpret_cast<RECT*>(lParam);
			switch (wParam) {
			case WMSZ_BOTTOM:
			case WMSZ_BOTTOMRIGHT:
			case WMSZ_BOTTOMLEFT:
				winRect->bottom -= (winRect->bottom - winRect->top - BorderHeight_) % tw->fontHeight_;
				break;
			default:
				winRect->top += (winRect->bottom - winRect->top - BorderHeight_) % tw->fontHeight_;
				break;
			}
			switch (wParam) {
			case WMSZ_RIGHT:
			case WMSZ_TOPRIGHT:
			case WMSZ_BOTTOMRIGHT:
				winRect->right -= (winRect->right - winRect->left - BorderWidth_) % tw->fontWidth_;
				break;
			default:
				winRect->left += (winRect->right - winRect->left - BorderWidth_) % tw->fontWidth_;
				break;
			}
			break;
		}
		/* Called when the window is resized to given values.

			No resize is performed if the window is minimized (we would have terminal size of length 0).

			It is ok if no terminal window is associated with the handle as the message can be sent from the WM_CREATE when window is resized to account for the window border which has to be calculated.
		 */
		case WM_SIZE: {
			if (wParam == SIZE_MINIMIZED)
				break;
			if (tw != nullptr) {
				RECT rect;
				GetClientRect(hWnd, &rect);
				tw->resize(rect.right, rect.bottom);
			}
			break;
		}
	    /* Repaint of the window is requested. */
		case WM_PAINT:
			ASSERT(tw != nullptr) << "Attempt to paint unknown window";
			// copy the memory buffer to the window area
			tw->doPaint();
			break;
		}
		return DefWindowProc(hWnd, msg, wParam, lParam);
	}

	void GDITerminalWindow::doPaint() {
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hWnd_, &ps);
		if (buffer_ == nullptr) {
			buffer_ = CreateCompatibleBitmap(hdc, width_, height_);
			SelectObject(memoryBuffer_, buffer_);
			// do the repaint 
			paintShadowBuffer(0, 0, cols(), rows());
		}
		BitBlt(hdc, 0, 0, width_, height_, memoryBuffer_, 0, 0, SRCCOPY);
		EndPaint(hWnd_, &ps);
	}

} // namespace tpp

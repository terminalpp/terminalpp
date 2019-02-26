#include "gdi_terminal_window.h"

namespace tpp {

	using namespace vterm;

	std::unordered_map<HWND, GDITerminalWindow *> GDITerminalWindow::Windows_;

	unsigned GDITerminalWindow::BorderWidth_ = 0;
	unsigned GDITerminalWindow::BorderHeight_ = 0;


	GDITerminalWindow::GDITerminalWindow(HWND hWnd) :
		hWnd_(hWnd),
	    buffer_(nullptr) {
		ASSERT(hWnd_ != nullptr);
		Windows_.insert(std::make_pair(hWnd_, this));
		// create the default font and determine its width
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hWnd_, &ps);
		memoryBuffer_ = CreateCompatibleDC(nullptr);
		HFONT defaultFont = getFont(vterm::Font());
		fontWidth_ = calculateFontWidth(hdc, defaultFont) + 2;
		fontHeight_ = Settings.fontHeight;
		EndPaint(hWnd_, &ps);
		// update the window size
		getWindowSize();
	}

	LRESULT CALLBACK GDITerminalWindow::EventHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
		auto i = Windows_.find(hWnd);
		GDITerminalWindow * tw = (i == Windows_.end()) ? nullptr : i->second;
		switch (msg) {
		/* When the window is created, the border width and height of a terminal window is determined and the window's size is updated to adjust for it. */
		case WM_CREATE: {
			CREATESTRUCT & cs = * reinterpret_cast<CREATESTRUCT*>(lParam);
			RECT r;
			r.left = cs.x;
			r.right = cs.x + cs.cx;
			r.top = cs.y;
			r.bottom = cs.y + cs.cy;
			AdjustWindowRectEx(&r, cs.style, false, cs.dwExStyle);
			unsigned bw = r.right - r.left - cs.cx;
			unsigned bh = r.bottom - r.top - cs.cy;
			ASSERT(BorderWidth_ == 0 || BorderWidth_ == bw) << "All terminal windows expected to have same border width";
			ASSERT(BorderHeight_ == 0 || BorderHeight_ == bh) << "All terminal windows expected to have same border height";
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
			PAINTSTRUCT ps;
			HDC hdc = BeginPaint(hWnd, &ps);
			if (tw->buffer_ == nullptr)
				tw->createBufferAndRepaint(hdc);
			BitBlt(hdc, 0, 0, tw->width_, tw->height_, tw->memoryBuffer_, 0, 0, SRCCOPY);
			EndPaint(hWnd, &ps);
			break;
		}
		return DefWindowProc(hWnd, msg, wParam, lParam);
	}

	HFONT GDITerminalWindow::getFont(vterm::Font const & font) {
		// see if the font variant has already been created
		auto i = fonts_.find(font);
		if (i != fonts_.end())
			return i->second;
		// if not, create the appropriate version of the font
		HFONT result = CreateFont(
			Settings.fontHeight * font.size(),
			0,
			0,
			0,
			font.bold() ? FW_BOLD : FW_DONTCARE,
			font.italics(),
			font.underline(),
			font.strikeout(),
			DEFAULT_CHARSET,
			OUT_OUTLINE_PRECIS,
			CLIP_DEFAULT_PRECIS,
			CLEARTYPE_QUALITY,
			FIXED_PITCH,
			Settings.fontName.c_str());
		// if the font creation failed, return the default font
		if (result == nullptr)
			return fonts_[vterm::Font()];
		// otherwise cache the font and return it
		return fonts_.insert(std::make_pair(font, result)).first->second;
	}

	unsigned GDITerminalWindow::calculateFontWidth(HDC hdc, HFONT font) {
		SelectObject(hdc, font);
		ABC abc;
		GetCharABCWidths(hdc, 'm', 'm', &abc);
		return abc.abcA + abc.abcB + abc.abcC;
	}

	void GDITerminalWindow::repaintTerminal(vterm::RepaintEvent & e) {
		// if the buffer is not valid, don't bother repainting, the WM_PAINT will call it when ready
		if (buffer_ == nullptr)
			return;
		ASSERT(e.sender == this || e.sender == terminal_) << "Unexpected trigger";
		{
			vterm::VirtualTerminal::ScreenBuffer sb(terminal_->screenBuffer());
			// if the screen buffer is empty, there is nothing to paint, exit
			if (sb.cols() == 0 || sb.rows() == 0)
				return;
			vterm::VirtualTerminal::ScreenCell const & firstCell = sb.at(0, 0);
			Color lastFg = firstCell.fg;
			Color lastBg = firstCell.bg;
			Font lastFont = firstCell.font;
//			HBRUSH brush = CreateSolidBrush(RGB(0, 0, 255));
//			RECT stuff;
//			SetRect(&stuff, 0, 0, 100, 100);
//			FillRect(memoryBuffer_, &stuff, brush);

			SetTextColor(memoryBuffer_, RGB(lastFg.red, lastFg.green, lastFg.blue));
			SetBkColor(memoryBuffer_, RGB(lastBg.red, lastBg.green, lastBg.blue));
			SelectObject(memoryBuffer_, getFont(lastFont));
			for (size_t r = e->top, re = e->top + e->rows; r != re; ++r) {
				for (size_t c = e->left, ce = e->left + e->cols; c != ce; ++c) {
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
					if (cell.font != lastFont) {
						lastFont = cell.font;
						SelectObject(memoryBuffer_, getFont(lastFont));
					}
					// draw the cell contents

					TextOutW(memoryBuffer_, c * fontWidth_, r * Settings.fontHeight, cell.c.w_str(), cell.c.size());
				}
			}
		}
		PostMessage(hWnd_, WM_PAINT, 0, 0);
	} 

	void GDITerminalWindow::resize(unsigned width, unsigned height) {
		if (width_ == width && height_ == height)
			return;
		// delete the old buffer and create a new buffer image of appropriate size
		if (buffer_ != nullptr)
			DeleteObject(buffer_);
		buffer_ = nullptr;
		TerminalWindow::resize(width, height);
	}

	void GDITerminalWindow::createBufferAndRepaint(HDC hdc) {
		// create the bitmap so that it copies the window properties properly
		ASSERT(buffer_ == nullptr) << "Buffer should be empty";
		buffer_ = CreateCompatibleBitmap(hdc, width_, height_);
		SelectObject(memoryBuffer_, buffer_);
		// do the repaint 
		repaintTerminal(vterm::RepaintEvent(this, vterm::TerminalRepaint{ 0,0, cols(), rows() }));
	}


	void GDITerminalWindow::getWindowSize() {
		RECT clientRect;
		GetClientRect(hWnd_, &clientRect);
		unsigned w = clientRect.right - clientRect.left;
		unsigned h = clientRect.bottom - clientRect.top;
		resize(w, h);
	}

} // namespace tpp

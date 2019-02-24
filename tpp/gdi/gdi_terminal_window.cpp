#include "gdi_terminal_window.h"

namespace tpp {

	using namespace vterm;

	std::unordered_map<HWND, GDITerminalWindow *> GDITerminalWindow::Windows_;

	GDITerminalWindow::GDITerminalWindow(HWND hWnd) :
		hWnd_(hWnd),
	    buffer_(nullptr) {
		ASSERT(hWnd_ != nullptr);
		Windows_.insert(std::make_pair(hWnd_, this));
		// create the default font and determine its width
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hWnd_, &ps);
		memoryBuffer_ = CreateCompatibleDC(hdc);
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
		case WM_CLOSE:
			DestroyWindow(hWnd);
			break;
		case WM_DESTROY:
			ASSERT(tw != nullptr) << "Attempt to destroy unknown window";
			Windows_.erase(i);
			if (Windows_.empty())
				PostQuitMessage(0);
			break;
		case WM_SIZE:
			tw->getWindowSize();
			break;
		case WM_PAINT:
			ASSERT(tw != nullptr) << "Attempt to paint unknown window";
			// copy the memory buffer to the window area
			PAINTSTRUCT ps;
			HDC hdc = BeginPaint(hWnd, &ps);
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
			DEFAULT_QUALITY,
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
		ASSERT(e.sender == this || e.sender == terminal_) << "Unexpected trigger";
		{
			Terminal::ScreenBuffer sb(terminal_->screenBuffer());
			Terminal::ScreenCell const & firstCell = sb.at(0, 0);
			Color lastFg = firstCell.fg;
			Color lastBg = firstCell.bg;
			Font lastFont = firstCell.font;
			SetTextColor(memoryBuffer_, RGB(lastFg.red, lastFg.green, lastFg.blue));
			SetBkColor(memoryBuffer_, RGB(lastBg.red, lastBg.green, lastBg.blue));
			SelectObject(memoryBuffer_, getFont(lastFont));
			for (size_t r = e->top, re = e->top + e->rows; r != re; ++r) {
				for (size_t c = e->left, ce = e->left + e->cols; c != ce; ++c) {
					// get the cell info
					Terminal::ScreenCell const & cell = sb.at(c, r);
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

	bool GDITerminalWindow::getWindowSize() {
		RECT clientRect;
		GetClientRect(hWnd_, &clientRect);
		size_t w = clientRect.right - clientRect.left;
		size_t h = clientRect.bottom - clientRect.top;
		if (w != width_ || h != height_) {
			width_ = w;
			height_ = h;
			if (terminal_ != nullptr)
				terminal_->resize(w / fontWidth_, h / fontHeight_);
			if (buffer_ != nullptr)
				DeleteObject(buffer_);
			buffer_ = CreateCompatibleBitmap(memoryBuffer_, width_, height_);
			SelectObject(memoryBuffer_, buffer_);
			return true;
		} else {
			return false;
		}
	}

} // namespace tpp

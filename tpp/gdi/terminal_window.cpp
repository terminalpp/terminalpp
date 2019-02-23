#include "terminal_window.h"

namespace tpp {

	using namespace vterm;

	std::unordered_map<HWND, TerminalWindow *> TerminalWindow::Windows_;

	TerminalWindow::TerminalWindow(HWND hWnd, Terminal * terminal) :
		hWnd_(hWnd),
		terminal_(terminal),
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
		// TODO delete this !!!!!
		bool bold = false;
		unsigned cp = 0xe000;
		{
			Terminal::ScreenBuffer buffer(terminal_->screenBuffer());
			for (size_t r = 0; r < buffer.rows(); ++r) {
				for (size_t c = 0; c < buffer.cols(); ++c) {
					Terminal::ScreenCell & cell = buffer.at(c, r);
					cell.fg = vterm::Color::White;
					cell.bg = vterm::Color::Black;
					cell.c = vterm::Char::UTF16((cp++ % 95) + 0xe000);
					cell.font.setBold(bold);
				}
				bold = !bold;
			}
		}
		refresh(0, 0, terminal_->cols(), terminal_->rows());
	}

	LRESULT CALLBACK TerminalWindow::EventHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
		auto i = Windows_.find(hwnd);
		TerminalWindow * tw = (i == Windows_.end()) ? nullptr : i->second;
		switch (msg) {
		case WM_CLOSE:
			DestroyWindow(hwnd);
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
			tw->doPaint();
			break;
		}
		return DefWindowProc(hwnd, msg, wParam, lParam);
	}

	HFONT TerminalWindow::getFont(vterm::Font const & font) {
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

	unsigned TerminalWindow::calculateFontWidth(HDC hdc, HFONT font) {
		SelectObject(hdc, font);
		ABC abc;
		GetCharABCWidths(hdc, 'm', 'm', &abc);
		return abc.abcA + abc.abcB + abc.abcC;
	}

	void TerminalWindow::doPaint() {
		// initialize the painting
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hWnd_, &ps);
		BitBlt(hdc, 0, 0, width_, height_, memoryBuffer_, 0, 0, SRCCOPY);
		// end the painting
		EndPaint(hWnd_, &ps);
	}

	void TerminalWindow::refresh(unsigned left, unsigned top, unsigned cols, unsigned rows) {
		Terminal::ScreenBuffer sb(terminal_->screenBuffer());
		Terminal::ScreenCell const & firstCell = sb.at(0, 0);
		Color lastFg = firstCell.fg;
		Color lastBg = firstCell.bg;
		Font lastFont = firstCell.font;
		SetTextColor(memoryBuffer_, RGB(lastFg.red, lastFg.green, lastFg.blue));
		SetBkColor(memoryBuffer_, RGB(lastBg.red, lastBg.green, lastBg.blue));
		SelectObject(memoryBuffer_, getFont(lastFont));
		for (size_t r = top, re = top + rows; r != re; ++r) {
			for (size_t c = left, ce = left + cols; c != ce; ++c) {
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

	bool TerminalWindow::getWindowSize() {
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

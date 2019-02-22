#include "terminal_window.h"

namespace tpp {

	std::unordered_map<HWND, TerminalWindow *> TerminalWindow::Windows_;

	TerminalWindow::TerminalWindow(HWND hwnd, vterm::ScreenBuffer * screenBuffer) :
		hwnd_(hwnd),
		screenBuffer_(screenBuffer) {
		ASSERT(hwnd_ != nullptr);
		Windows_.insert(std::make_pair(hwnd_, this));
		// create the default font and determine its width
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hwnd_, &ps);
		HFONT defaultFont = getFont(vterm::Font());
		fontWidth_ = calculateFontWidth(hdc, defaultFont);
		EndPaint(hwnd_, &ps);
		// 
		screenBuffer_->resize(100, 100);
		bool bold = false;
		for (size_t r = 0; r < 100; ++r) {
			for (size_t c = 0; c < 100; ++c) {
				vterm::ScreenBuffer::Cell & cell = screenBuffer_->at(c, r);
				cell.fg = vterm::Color::White;
				cell.bg = vterm::Color::Black;
				cell.c = vterm::Char((char)(' ' + c));
				cell.font.setBold(bold);
			}
			bold = !bold;
		}
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
			CLEARTYPE_QUALITY,
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
		HDC hdc = BeginPaint(hwnd_, &ps);
		vterm::ScreenBuffer & sb = *screenBuffer_;
		vterm::ScreenBuffer::Cell const & firstCell = sb.at(0, 0);
		vterm::Color lastFg = firstCell.fg;
		vterm::Color lastBg = firstCell.bg;
		vterm::Font lastFont = firstCell.font;
		SetTextColor(hdc, RGB(lastFg.red, lastFg.green, lastFg.blue));
		SetBkColor(hdc, RGB(lastBg.red, lastBg.green, lastBg.blue));
		SelectObject(hdc, getFont(lastFont));
		for (size_t c = 0, ce = sb.cols(); c != ce; ++c) {
			for (size_t r = 0, re = sb.rows(); r != re; ++r) {
				// get the cell info
				vterm::ScreenBuffer::Cell const & cell = sb.at(c, r);
				// update rendering properties if necessary
				if (cell.fg != lastFg) {
					lastFg = cell.fg;
					SetTextColor(hdc, RGB(lastFg.red, lastFg.green, lastFg.blue));
				}
				if (cell.bg != lastBg) {
					lastBg = cell.bg;
					SetBkColor(hdc, RGB(lastBg.red, lastBg.green, lastBg.blue));
				}
				if (cell.font != lastFont) {
					lastFont = cell.font;
					SelectObject(hdc, getFont(lastFont));
				}
				// draw the cell contents
				TextOutW(hdc, c * fontWidth_, r * Settings.fontHeight, L"k", 1);
			}
		}
		// end the painting
		EndPaint(hwnd_, &ps);
	}



} // namespace tpp

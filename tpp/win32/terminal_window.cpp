#include "application.h"

#include "terminal_window.h"



namespace tpp {

	std::unordered_map<HWND, TerminalWindow *> TerminalWindow::Windows_;

	TerminalWindow::TerminalWindow(Application * app, TerminalSettings * settings) :
		BaseTerminalWindow{FillPlatformSettings(settings)},
		bufferDC_(CreateCompatibleDC(nullptr)),
		buffer_(nullptr),
		frameWidth_{0},
		frameHeight_{0} {
		hWnd_ = CreateWindowEx(
			WS_EX_LEFT, // the default
			app->TerminalWindowClassName_, // window class
			name_.c_str(), // window name (all start as terminal++)
			WS_OVERLAPPEDWINDOW,
			CW_USEDEFAULT, // x position
			CW_USEDEFAULT, // y position
			widthPx_,
			heightPx_,
			nullptr, // handle to parent
			nullptr, // handle to menu - TODO I should actually create a menu
			app->hInstance_, // module handle
			this // lParam for WM_CREATE message
		);
		ASSERT(hWnd_ != 0) << "Cannot create window : " << GetLastError();
		Windows_.insert(std::make_pair(hWnd_, this));
	}

	TerminalWindow::~TerminalWindow() {
		DeleteObject(buffer_);
		DeleteObject(bufferDC_);
	}

	void TerminalWindow::repaint(vterm::Terminal::RepaintEvent & e) {
		// do not bother with repainting if shadow buffer is invalid, the WM_PAINT will redraw the whole buffer when the redraw is processed
		if (buffer_ == nullptr)
			return;
		updateBuffer(*e);
		InvalidateRect(hWnd_, /* rect */ nullptr, /* erase */ false);
	}

	void TerminalWindow::updateBuffer(helpers::Rect const & rect) {
		if (terminal() == nullptr)
			return;
		vterm::Terminal::Layer l = terminal()->getDefaultLayer();
		HFONT hFont = Font::GetOrCreate(vterm::Font(), settings_->defaultFontHeight, zoom_)->handle();
		SetTextColor(bufferDC_, RGB(255, 255, 255));
		SetBkColor(bufferDC_, RGB(0,0,0));
		SelectObject(bufferDC_, hFont);
		for (unsigned col = rect.left; col < rect.right; ++col) {
			for (unsigned row = rect.top; row < rect.bottom; ++row) {
				vterm::Cell & c = l->at(col, row);
				// ISSUE 4 - make sure we can print full Unicode, not just UCS2
				wchar_t wc = L'3';//c.c.toWChar();
				TextOutW(bufferDC_, col * cellWidthPx_, row * cellHeightPx_, &wc, 1);
			}
		}
	}

	TerminalSettings * TerminalWindow::FillPlatformSettings(TerminalSettings * ts) {
		vterm::Font defaultFont;
		Font * f = Font::GetOrCreate(defaultFont, ts->defaultFontHeight, 1);
		defaultFont.setBold(true);
		Font * bf = Font::GetOrCreate(defaultFont, ts->defaultFontHeight, 1);
		ts->defaultFontWidth = std::max(f->widthPx(), bf->widthPx());
		return ts;
	}

	LRESULT CALLBACK TerminalWindow::EventHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
		// determine terminal window corresponding to the handle given with the message
		auto i = Windows_.find(hWnd);
		TerminalWindow * tw = i == Windows_.end() ? nullptr : i->second;
		// do the message
		switch (msg) {
			/** Closes the current window. */
			case WM_CLOSE: {
				DestroyWindow(hWnd);
				break;
			}
			/** Destroys the current window, if it is last window of the application, we quit for now. */
			case WM_DESTROY: {
				ASSERT(tw != nullptr) << "Attempt to destroy unknown window";
				// delete the window object
				delete i->second;
				// remove the window from the list of windows
				Windows_.erase(i);
				// if it was last window, terminate the application
				if (Windows_.empty())
					PostQuitMessage(0);
				break;
			}
			/* When the window is created, the border width and height of a terminal window is determined and the window's size is updated to adjust for it. */
			case WM_CREATE: {
				ASSERT(tw == nullptr);
				tw = reinterpret_cast<TerminalWindow*>(lParam);
				ASSERT(tw != nullptr);
				// now calculate the border and actually update the window size to account for it
				CREATESTRUCT & cs = *reinterpret_cast<CREATESTRUCT*>(lParam);
				RECT r;
				r.left = cs.x;
				r.right = cs.x + cs.cx;
				r.top = cs.y;
				r.bottom = cs.y + cs.cy;
				AdjustWindowRectEx(&r, cs.style, false, cs.dwExStyle);
				unsigned fw = r.right - r.left - cs.cx;
				unsigned fh = r.bottom - r.top - cs.cy;
				if (fw != 0 || fh != 0) {
					tw->frameWidth_ = fw;
					tw->frameHeight_ = fh;
					SetWindowPos(hWnd, HWND_TOP, cs.x, cs.y, cs.cx + fw, cs.cy + fh, SWP_NOZORDER);
				}
				break;
			}
			/* Repaint of the window is requested. */
			case WM_PAINT: {
				ASSERT(tw != nullptr) << "Attempt to paint unknown window";
				PAINTSTRUCT ps;
				HDC hdc = BeginPaint(hWnd, &ps);
				if (tw->buffer_ == nullptr) {
					tw->buffer_ = CreateCompatibleBitmap(hdc, tw->widthPx_, tw->heightPx_);
					SelectObject(tw->bufferDC_, tw->buffer_);
					// do the repaint 
					tw->updateBuffer(helpers::Rect(0,0,tw->cols(), tw->rows()));
				}
				BitBlt(hdc, 0, 0, tw->widthPx_, tw->heightPx_, tw->bufferDC_, 0, 0, SRCCOPY);
				EndPaint(hWnd, &ps);
				break;
        	}
		} // end of switch
		return DefWindowProc(hWnd, msg, wParam, lParam);
	}

} // namespace tpp
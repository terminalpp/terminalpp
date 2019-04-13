#include "application.h"

#include "terminal_window.h"



namespace tpp {

	std::unordered_map<HWND, TerminalWindow *> TerminalWindow::Windows_;

	TerminalWindow::TerminalWindow(Application * app, TerminalSettings * settings) :
		BaseTerminalWindow{FillPlatformSettings(settings)},
		bufferDC_(CreateCompatibleDC(nullptr)),
		buffer_(nullptr),
		updateRect_(0,0),
		wndPlacement_{sizeof(wndPlacement_)},
		frameWidth_{0},
		frameHeight_{0} {
		hWnd_ = CreateWindowEx(
			WS_EX_LEFT, // the default
			app->TerminalWindowClassName_, // window class
			title_.c_str(), // window name (all start as terminal++)
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

	void TerminalWindow::resizeWindow(unsigned width, unsigned height) {
		// invalidate the buffer
		DeleteObject(buffer_);
		buffer_ = nullptr;
		// resize the window
		BaseTerminalWindow::resizeWindow(width, height);
	}

	void TerminalWindow::repaint(vterm::Terminal::RepaintEvent & e) {
		// do not bother with repainting if shadow buffer is invalid, the WM_PAINT will redraw the whole buffer when the redraw is processed
		if (buffer_ == nullptr)
			return;
		updateRect_ = helpers::Rect::Union(*e, updateRect_);
		InvalidateRect(hWnd_, /* rect */ nullptr, /* erase */ false);
	}

	/** Basically taken from:

	    https://devblogs.microsoft.com/oldnewthing/20100412-00/?p=14353
	 */
	void TerminalWindow::doSetFullscreen(bool value) {
		DWORD style = GetWindowLong(hWnd_, GWL_STYLE);
		if (value == true) {
			MONITORINFO mInfo = { sizeof(mInfo) };
			if (GetWindowPlacement(hWnd_, &wndPlacement_) &&
				GetMonitorInfo(MonitorFromWindow(hWnd_, MONITOR_DEFAULTTOPRIMARY), &mInfo)) {
				SetWindowLong(hWnd_, GWL_STYLE, style & ~WS_OVERLAPPEDWINDOW);
				int width = mInfo.rcMonitor.right - mInfo.rcMonitor.left;
				int height = mInfo.rcMonitor.bottom - mInfo.rcMonitor.top;
				SetWindowPos(hWnd_, HWND_TOP, mInfo.rcMonitor.left, mInfo.rcMonitor.top, width, height, SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
			} else {
				// we are not actually fullscreen
				fullscreen_ = false;
				LOG("Win32") << "Unable to enter fullscreen mode";
			}
		} else {
			SetWindowLong(hWnd_, GWL_STYLE, style | WS_OVERLAPPEDWINDOW);
			SetWindowPlacement(hWnd_, &wndPlacement_);
			SetWindowPos(hWnd_, nullptr, 0, 0, 0, 0,
				SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER |
				SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
		}
	}

	void TerminalWindow::doTitleChange(vterm::VT100::TitleEvent & e) {
		if (title_ != *e) {
			title_ = *e;
			PostMessage(hWnd_, WM_USER, MSG_TITLE_CHANGE, 0);
		}
	}

	void TerminalWindow::updateBuffer() {
		if (terminal() == nullptr)
			return;
		vterm::Terminal::Layer l = terminal()->getDefaultLayer();
		// initialize the font & colors
		vterm::Cell & c = l->at(updateRect_.left, updateRect_.top);
		vterm::Color currentFg = c.fg;
		vterm::Color currentBg = c.bg;
		vterm::Font currentFont = DropBlink(c.font);
		SetTextColor(bufferDC_, RGB(currentFg.red, currentFg.green, currentFg.blue));
		SetBkColor(bufferDC_, RGB(currentBg.red, currentBg.green,currentBg.blue));
		SelectObject(bufferDC_, Font::GetOrCreate(currentFont, settings_->defaultFontHeight, zoom_)->handle());
		for (unsigned row = updateRect_.top; row < updateRect_.bottom; ++row) {
			for (unsigned col = updateRect_.left; col < updateRect_.right; ++col) {
				vterm::Cell & c = l->at(col, row);
				if (currentFg != c.fg) {
					currentFg = c.fg;
					SetTextColor(bufferDC_, RGB(currentFg.red, currentFg.green, currentFg.blue));
				}
				if (currentBg != c.bg) {
					currentBg = c.bg;
					SetBkColor(bufferDC_, RGB(currentBg.red, currentBg.green, currentBg.blue));
				}
				if (currentFont != DropBlink(c.font)) {
					currentFont = DropBlink(c.font);
					SelectObject(bufferDC_, Font::GetOrCreate(currentFont, settings_->defaultFontHeight, zoom_)->handle());
				}
				// ISSUE 4 - make sure we can print full Unicode, not just UCS2 - perhaps not possible with GDI alone
				wchar_t wc = c.c.toWChar();
				TextOutW(bufferDC_, col * cellWidthPx_, row * cellHeightPx_, &wc, 1);
			}
		}
		updateRect_ = helpers::Rect(0,0);
	}

	TerminalSettings * TerminalWindow::FillPlatformSettings(TerminalSettings * ts) {
		vterm::Font defaultFont;
		Font * f = Font::GetOrCreate(defaultFont, ts->defaultFontHeight, 1);
		defaultFont.setBold(true);
		Font * bf = Font::GetOrCreate(defaultFont, ts->defaultFontHeight, 1);
		ts->defaultFontWidth = std::max(f->widthPx(), bf->widthPx());
		return ts;
	}

	// https://docs.microsoft.com/en-us/windows/desktop/inputdev/virtual-key-codes
	vterm::Key TerminalWindow::GetKey(WPARAM vk) {
		if (!vterm::Key::IsValidCode(vk))
			return vterm::Key(vterm::Key::None);
		// MSB == pressed, LSB state since last time
		unsigned shift = (GetAsyncKeyState(VK_SHIFT) & 0x8000) ? vterm::Key::Shift : 0;
		unsigned ctrl = (GetAsyncKeyState(VK_CONTROL) & 0x8000) ? vterm::Key::Ctrl : 0;
		unsigned alt = (GetAsyncKeyState(VK_MENU) & 0x8000) ? vterm::Key::Alt : 0;
		unsigned win = (GetAsyncKeyState(VK_LWIN) & 0x8000) || (GetAsyncKeyState(VK_RWIN) & 0x8000) ? vterm::Key::Meta : 0;

		return vterm::Key(vk, shift | ctrl | alt | win);
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
				// now calculate the border and actually update the window size to account for it
				CREATESTRUCT & cs = *reinterpret_cast<CREATESTRUCT*>(lParam);
				// get the tw member from the create struct argument
				ASSERT(tw == nullptr);
				tw = reinterpret_cast<TerminalWindow*>(cs.lpCreateParams);
				ASSERT(tw != nullptr);
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
			/* Called when the window is resized interactively by the user. Makes sure that the window size snaps to discrete terminal sizes. */
			case WM_SIZING: {
				RECT * winRect = reinterpret_cast<RECT*>(lParam);
				switch (wParam) {
				case WMSZ_BOTTOM:
				case WMSZ_BOTTOMRIGHT:
				case WMSZ_BOTTOMLEFT:
					winRect->bottom -= (winRect->bottom - winRect->top - tw->frameHeight_) % tw->cellHeightPx_;
					break;
				default:
					winRect->top += (winRect->bottom - winRect->top - tw->frameHeight_) % tw->cellHeightPx_;
					break;
				}
				switch (wParam) {
				case WMSZ_RIGHT:
				case WMSZ_TOPRIGHT:
				case WMSZ_BOTTOMRIGHT:
					winRect->right -= (winRect->right - winRect->left - tw->frameWidth_) % tw->cellWidthPx_;
					break;
				default:
					winRect->left += (winRect->right - winRect->left - tw->frameWidth_) % tw->cellWidthPx_;
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
					tw->resizeWindow(rect.right, rect.bottom);
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
					tw->updateRect_ = helpers::Rect(tw->cols(), tw->rows());
				}
				// do the repaint 
				if (!tw->updateRect_.empty())
					tw->updateBuffer();
				BitBlt(hdc, 0, 0, tw->widthPx_, tw->heightPx_, tw->bufferDC_, 0, 0, SRCCOPY);
				EndPaint(hWnd, &ps);
				break;
        	}
			/* TODO It would be nice to actually switch to unicode. */
			case WM_UNICHAR:
				UNREACHABLE;
				break;
			case WM_CHAR:
				if (wParam >= 0x20)
				    tw->terminal()->charInput(vterm::Char::UTF8(static_cast<unsigned>(wParam)));
				break;
			/* DEBUG - debugging events hooked to keypresses now: */
			/* Processes special key events.*/
			case WM_SYSKEYDOWN:
			case WM_KEYDOWN: {
				vterm::Key k = GetKey(wParam);
				if (k == (vterm::Key::Enter | vterm::Key::Alt)) {
					tw->setFullscreen(!tw->fullscreen());
				} else if (k == vterm::Key::F5) {
					tw->redraw();
				} else if (k != vterm::Key::None) {
					tw->terminal()->keyDown(k);
				}
				break;
			}
			case WM_KEYUP: {
				vterm::Key k = GetKey(wParam);
				tw->terminal()->keyUp(k);
				break;
			}
			/* User specified messages for various events that we want to be handled in the app thread.
			 */
			case WM_USER:
				switch (wParam) {
				case MSG_TITLE_CHANGE:
					SetWindowTextA(hWnd, tw->title().c_str());
					break;
				default:
					LOG("Win32") << "Invalid user message " << wParam;
				}
				break;
		} // end of switch
		return DefWindowProc(hWnd, msg, wParam, lParam);
	}

} // namespace tpp
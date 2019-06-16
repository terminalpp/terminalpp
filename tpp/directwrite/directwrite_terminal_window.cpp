#ifdef WIN32

#include "helpers/strings.h"


#include "../session.h"

#include "directwrite_application.h"

#include "directwrite_terminal_window.h"



namespace tpp {

	std::unordered_map<HWND, DirectWriteTerminalWindow*> DirectWriteTerminalWindow::Windows_;

	DirectWriteTerminalWindow::DirectWriteTerminalWindow(Session* session, Properties const& properties, std::string const& title) :
		TerminalWindow(session, properties, title),
		wndPlacement_{ sizeof(wndPlacement_) },
		frameWidth_{ 0 },
		frameHeight_{ 0 } {
		hWnd_ = CreateWindowEx(
			WS_EX_LEFT, // the default
			app()->TerminalWindowClassName_, // window class
			title_.c_str(), // window name (all start as terminal++)
			WS_OVERLAPPEDWINDOW,
			CW_USEDEFAULT, // x position
			CW_USEDEFAULT, // y position
			widthPx_,
			heightPx_,
			nullptr, // handle to parent
			nullptr, // handle to menu - TODO I should actually create a menu
			app()->hInstance_, // module handle
			this // lParam for WM_CREATE message
		);
		ASSERT(hWnd_ != 0) << "Cannot create window : " << GetLastError();
		SetTimer(hWnd_, TIMERID_BLINK, 500, nullptr);
		Windows_.insert(std::make_pair(hWnd_, this));
	}

	DirectWriteTerminalWindow::~DirectWriteTerminalWindow() {
		Windows_.erase(hWnd_);
	}

	/** Basically taken from:

		https://devblogs.microsoft.com/oldnewthing/20100412-00/?p=14353
	 */
	void DirectWriteTerminalWindow::doSetFullscreen(bool value) {
		DWORD style = GetWindowLong(hWnd_, GWL_STYLE);
		if (value == true) {
			MONITORINFO mInfo = { sizeof(mInfo) };
			if (GetWindowPlacement(hWnd_, &wndPlacement_) &&
				GetMonitorInfo(MonitorFromWindow(hWnd_, MONITOR_DEFAULTTOPRIMARY), &mInfo)) {
				SetWindowLong(hWnd_, GWL_STYLE, style & ~WS_OVERLAPPEDWINDOW);
				int width = mInfo.rcMonitor.right - mInfo.rcMonitor.left;
				int height = mInfo.rcMonitor.bottom - mInfo.rcMonitor.top;
				SetWindowPos(hWnd_, HWND_TOP, mInfo.rcMonitor.left, mInfo.rcMonitor.top, width, height, SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
			}
			else {
				// we are not actually fullscreen
				fullscreen_ = false;
				LOG("Win32") << "Unable to enter fullscreen mode";
			}
		}
		else {
			SetWindowLong(hWnd_, GWL_STYLE, style | WS_OVERLAPPEDWINDOW);
			SetWindowPlacement(hWnd_, &wndPlacement_);
			SetWindowPos(hWnd_, nullptr, 0, 0, 0, 0,
				SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER |
				SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
		}
	}

	void DirectWriteTerminalWindow::titleChange(vterm::Terminal::TitleChangeEvent& e) {
		title_ = *e;
		PostMessage(hWnd_, WM_USER, MSG_TITLE_CHANGE, 0);
	}

	void DirectWriteTerminalWindow::clipboardPaste() {
		if (OpenClipboard(nullptr)) {
			HANDLE clipboard = GetClipboardData(CF_UNICODETEXT);
			if (clipboard) {
				WCHAR* data = reinterpret_cast<WCHAR*>(GlobalLock(clipboard));
				if (data) {
					std::string str(helpers::UTF16toUTF8(data));
					GlobalUnlock(clipboard);
					if (!str.empty())
						terminal()->paste(str);
				}
			}
			CloseClipboard();
		}
	}

	void DirectWriteTerminalWindow::clipboardCopy(std::string const& str) {
		if (OpenClipboard(nullptr)) {
			EmptyClipboard();
			// encode the string into UTF16 and get the size of the data we need
			NOT_IMPLEMENTED;
			size_t size = 0;
			HGLOBAL clipboard = GlobalAlloc(0, size);
			if (clipboard) {
				WCHAR* data = reinterpret_cast<WCHAR*>(GlobalLock(clipboard));
				if (data) {
					// TODO copy the memory
					GlobalUnlock(clipboard);
					SetClipboardData(CF_UNICODETEXT, clipboard);
				}
			}
			CloseClipboard();
		}
	}


	void DirectWriteTerminalWindow::doPaint() {
		RECT rc;
		GetClientRect(hWnd_, &rc);

		D2D1_SIZE_U size = D2D1::SizeU(rc.right - rc.left, rc.bottom - rc.top);
		OSCHECK(SUCCEEDED(app()->d2dFactory_->CreateHwndRenderTarget(
			D2D1::RenderTargetProperties(),
			D2D1::HwndRenderTargetProperties(
				hWnd_,
				size
			),
			&rt_
		)));

		OSCHECK(SUCCEEDED(rt_->CreateSolidColorBrush(
			D2D1::ColorF(D2D1::ColorF::White),
			&fg_
		)));
		OSCHECK(SUCCEEDED(rt_->CreateSolidColorBrush(
			D2D1::ColorF(D2D1::ColorF::Black),
			&bg_
		)));
		rt_->BeginDraw();

		rt_->SetTransform(D2D1::IdentityMatrix());

		rt_->Clear(D2D1::ColorF(D2D1::ColorF::White));

		doUpdateBuffer(true);
		rt_->EndDraw();
		fg_->Release();
		bg_->Release();
		rt_->Release();


		/*
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hWnd_, &ps);
		bool forceDirty = false;
		if (invalidated_ && buffer_ != nullptr) {
			DeleteObject(buffer_);
			buffer_ = nullptr;
		}
		if (buffer_ == nullptr) {
			buffer_ = CreateCompatibleBitmap(hdc, widthPx_, heightPx_);
			SelectObject(bufferDC_, buffer_);
			forceDirty = true;
			invalidated_ = false;
		}
		SetBkMode(bufferDC_, OPAQUE);
		// check if we ned to repaint any cells
		doUpdateBuffer(forceDirty);
		// copy the shadow image
		BitBlt(hdc, 0, 0, widthPx_, heightPx_, bufferDC_, 0, 0, SRCCOPY);
		EndPaint(hWnd_, &ps);
		*/
	}

	// https://docs.microsoft.com/en-us/windows/desktop/inputdev/virtual-key-codes
	vterm::Key DirectWriteTerminalWindow::GetKey(WPARAM vk) {
		if (!vterm::Key::IsValidCode(vk))
			return vterm::Key(vterm::Key::Invalid);
		// MSB == pressed, LSB state since last time
		unsigned shift = (GetAsyncKeyState(VK_SHIFT) & 0x8000) ? vterm::Key::Shift : 0;
		unsigned ctrl = (GetAsyncKeyState(VK_CONTROL) & 0x8000) ? vterm::Key::Ctrl : 0;
		unsigned alt = (GetAsyncKeyState(VK_MENU) & 0x8000) ? vterm::Key::Alt : 0;
		unsigned win = (GetAsyncKeyState(VK_LWIN) & 0x8000) || (GetAsyncKeyState(VK_RWIN) & 0x8000) ? vterm::Key::Win : 0;

		return vterm::Key(vk, shift | ctrl | alt | win);
	}

	LRESULT CALLBACK DirectWriteTerminalWindow::EventHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
		// determine terminal window corresponding to the handle given with the message
		auto i = Windows_.find(hWnd);
		DirectWriteTerminalWindow* tw = i == Windows_.end() ? nullptr : i->second;
		// do the message
		switch (msg) {
			/** Closes the current window. */
		case WM_CLOSE: {
			ASSERT(tw != nullptr) << "Unknown window";
			Session::Close(tw->session());
			break;
		}
					   /** Destroys the window, if it is the last window, quits the app. */
		case WM_DESTROY: {
			ASSERT(tw != nullptr) << "Attempt to destroy unknown window";
			// delete the window object and remove it from the list of active windows
			delete i->second;
			// if it was last window, terminate the application
			if (Windows_.empty())
				PostQuitMessage(0);
			break;
		}
						 /* When the window is created, the border width and height of a terminal window is determined and the window's size is updated to adjust for it. */
		case WM_CREATE: {
			// now calculate the border and actually update the window size to account for it
			CREATESTRUCT& cs = *reinterpret_cast<CREATESTRUCT*>(lParam);
			// get the tw member from the create struct argument
			ASSERT(tw == nullptr);
			tw = reinterpret_cast<DirectWriteTerminalWindow*>(cs.lpCreateParams);
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
			RECT* winRect = reinterpret_cast<RECT*>(lParam);
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
			tw->doPaint();
			break;
		}
					   /* TODO It would be nice to actually switch to unicode. */
		case WM_UNICHAR:
			UNREACHABLE;
			break;
		case WM_CHAR:
			if (wParam >= 0x20)
				tw->keyChar(vterm::Char::UTF8(static_cast<unsigned>(wParam)));
			break;
			/* DEBUG - debugging events hooked to keypresses now: */
			/* Processes special key events.*/
		case WM_SYSKEYDOWN:
		case WM_KEYDOWN: {
			vterm::Key k = GetKey(wParam);
			if (k != vterm::Key::Invalid)
				tw->keyDown(k);
			break;
		}
		case WM_KEYUP: {
			vterm::Key k = GetKey(wParam);
			tw->keyUp(k);
			break;
		}
					   /* Mouse events which simply obtain the mouse coordinates, convert the buttons and wheel values to vterm standards and then calls the DirectWriteTerminalWindow's events, which perform the pixels to cols & rows translation and then call the terminal itself.
						*/
		case WM_LBUTTONDOWN:
			tw->mouseDown(lParam & 0xffff, lParam >> 16, vterm::MouseButton::Left);
			break;
		case WM_LBUTTONUP:
			tw->mouseUp(lParam & 0xffff, lParam >> 16, vterm::MouseButton::Left);
			break;
		case WM_RBUTTONDOWN:
			tw->mouseDown(lParam & 0xffff, lParam >> 16, vterm::MouseButton::Right);
			break;
		case WM_RBUTTONUP:
			tw->mouseUp(lParam & 0xffff, lParam >> 16, vterm::MouseButton::Right);
			break;
		case WM_MBUTTONDOWN:
			tw->mouseDown(lParam & 0xffff, lParam >> 16, vterm::MouseButton::Wheel);
			break;
		case WM_MBUTTONUP:
			tw->mouseUp(lParam & 0xffff, lParam >> 16, vterm::MouseButton::Wheel);
			break;
		case WM_MOUSEWHEEL:
			tw->mouseWheel(lParam & 0xffff, lParam >> 16, GET_WHEEL_DELTA_WPARAM(wParam) / WHEEL_DELTA);
			break;
		case WM_MOUSEMOVE:
			tw->mouseMove(lParam & 0xffff, lParam >> 16);
			break;
			/* Timer even for blink text and cursor.
			 */
		case WM_TIMER: {
			if (wParam == TIMERID_BLINK) {
				tw->blink_ = !tw->blink_;
				InvalidateRect(hWnd, nullptr, /* erase */ false);
			}
			break;
		}
					   /* User specified messages for various events that we want to be handled in the app thread.
						*/
		case WM_USER:
			switch (wParam) {
			case MSG_TITLE_CHANGE:
				SetWindowTextA(hWnd, tw->terminal()->title().c_str());
				break;
			case MSG_INPUT_READY:
				tw->session()->processInput();
				break;
			default:
				LOG("Win32") << "Invalid user message " << wParam;
			}
			break;
		} // end of switch
		return DefWindowProc(hWnd, msg, wParam, lParam);
	}

} // namespace tpp

#endif
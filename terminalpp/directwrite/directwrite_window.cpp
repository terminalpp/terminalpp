#if (defined ARCH_WINDOWS && defined RENDERER_NATIVE)
#include "helpers/log.h"

#include "directwrite_window.h"

#include "directwrite_font.h"

namespace tpp2 {

    void DirectWriteWindow::setTitle(std::string const & value) {
        if (title() != value) {
            RendererWindow::setTitle(value);
            // actually change the title since we are in the UI thread now
            helpers::utf16_string t = helpers::UTF8toUTF16(value);
            // ok, on windows wchar_t and char16_t are the same (see helpers/char.h)
            SetWindowTextW(hWnd_, t.c_str());
        }
    }

    void DirectWriteWindow::setFullscreen(bool value) {
        if (fullscreen() != value) {
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
                    return;
                }
            } else {
                SetWindowLong(hWnd_, GWL_STYLE, style | WS_OVERLAPPEDWINDOW);
                SetWindowPlacement(hWnd_, &wndPlacement_);
                SetWindowPos(hWnd_, nullptr, 0, 0, 0, 0,
                    SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER |
                    SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
            }
            RendererWindow::setFullscreen(value);
        }
    }



    DirectWriteWindow::DirectWriteWindow(std::string const & title, int cols, int rows):
        RendererWindow<DirectWriteWindow, HWND>{cols, rows, Font{}, 1.0},
        wndPlacement_{ sizeof(wndPlacement_) },
        frameWidth_{0},
        frameHeight_{0} {
        // create the window with given title
        helpers::utf16_string t = helpers::UTF8toUTF16(title);
        hWnd_ = CreateWindowExW(
            WS_EX_LEFT, // the default
            DirectWriteApplication::Instance()->WindowClassName_, // window class
            // ok, on windows wchar_t and char16_t are the same (see helpers/char.h)
            t.c_str(), // window name (all start as terminal++)
            WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT, // x position
            CW_USEDEFAULT, // y position
            widthPx(),
            heightPx(),
            nullptr, // handle to parent
            nullptr, // handle to menu 
            DirectWriteApplication::Instance()->hInstance_, // module handle
            this // lParam for WM_CREATE message
        );
        // initialize the rendering structures


        // register the window
        RegisterWindowHandle(this, hWnd_);        
    }

	// https://docs.microsoft.com/en-us/windows/desktop/inputdev/virtual-key-codes
	Key DirectWriteWindow::GetKey(unsigned vk) {
		// we don't distinguish between left and right win keys
		if (vk == VK_RWIN)
			vk = VK_LWIN;
		if (! Key::IsValidCode(vk))
			return Key(Key::Invalid);
		// MSB == pressed, LSB state since last time
		unsigned shift = (GetAsyncKeyState(VK_SHIFT) & 0x8000) ? ui::Key::Shift : 0;
		unsigned ctrl = (GetAsyncKeyState(VK_CONTROL) & 0x8000) ? ui::Key::Ctrl : 0;
		unsigned alt = (GetAsyncKeyState(VK_MENU) & 0x8000) ? ui::Key::Alt : 0;
		unsigned win = (GetAsyncKeyState(VK_LWIN) & 0x8000) || (GetAsyncKeyState(VK_RWIN) & 0x8000) ? ui::Key::Win : 0;

		return Key(vk, shift | ctrl | alt | win);
	}

	LRESULT CALLBACK DirectWriteWindow::EventHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        DirectWriteWindow * window = GetWindowForHandle(hWnd);
        switch (msg) {
			/* When the window is created, the border width and height of a terminal window is determined and the window's size is updated to adjust for it. */
			case WM_CREATE: {
				// now calculate the border and actually update the window size to account for it
				CREATESTRUCT& cs = *reinterpret_cast<CREATESTRUCT*>(lParam);
				// get the tw member from the create struct argument
				ASSERT(window == nullptr);
				window = reinterpret_cast<DirectWriteWindow*>(cs.lpCreateParams);
				ASSERT(window != nullptr);
				RECT r;
				r.left = cs.x;
				r.right = cs.x + cs.cx;
				r.top = cs.y;
				r.bottom = cs.y + cs.cy;
				AdjustWindowRectEx(&r, cs.style, false, cs.dwExStyle);
				unsigned fw = r.right - r.left - cs.cx;
				unsigned fh = r.bottom - r.top - cs.cy;
				if (fw != 0 || fh != 0) {
					window->frameWidth_ = fw;
					window->frameHeight_ = fh;
					SetWindowPos(hWnd, HWND_TOP, cs.x, cs.y, cs.cx + fw, cs.cy + fh, SWP_NOZORDER | SWP_NOACTIVATE);
				}
				break;
			}
            /** Window gains keyboard focus. 
			 */
			case WM_SETFOCUS:
				if (window != nullptr)
    				window->rendererFocusIn();
				break;
			/** Window loses keyboard focus. 
			 */
			case WM_KILLFOCUS:
				if (window != nullptr)
    				window->rendererFocusOut();
				break;
			/* Called when the window is resized interactively by the user. Makes sure that the window size snaps to discrete terminal sizes. */
			case WM_SIZING: {
				RECT* winRect = reinterpret_cast<RECT*>(lParam);
				switch (wParam) {
                    case WMSZ_BOTTOM:
                    case WMSZ_BOTTOMRIGHT:
                    case WMSZ_BOTTOMLEFT:
                        winRect->bottom -= (winRect->bottom - winRect->top - window->frameHeight_) % window->cellHeight_;
                        break;
                    default:
                        winRect->top += (winRect->bottom - winRect->top - window->frameHeight_) % window->cellHeight_;
                        break;
				}
				switch (wParam) {
                    case WMSZ_RIGHT:
                    case WMSZ_TOPRIGHT:
                    case WMSZ_BOTTOMRIGHT:
                        winRect->right -= (winRect->right - winRect->left - window->frameWidth_) % window->cellWidth_;
                        break;
                    default:
                        winRect->left += (winRect->right - winRect->left - window->frameWidth_) % window->cellWidth_;
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
				if (window != nullptr) {
					RECT rect;
					GetClientRect(hWnd, &rect);
					window->windowResized(rect.right, rect.bottom);
				}
				break;
			}
			/* Repaint of the window is requested. 
             */
			case WM_PAINT: {
				ASSERT(window != nullptr) << "Attempt to paint unknown window";
				window->render(Rect::FromWH(window->width(), window->height()));
				break;
			}
			/* No need to use WM_UNICHAR since WM_CHAR is already unicode aware */
			case WM_UNICHAR:
				UNREACHABLE;
				break;
			/* If a system character is intercepted, do nothing & bypass the default implementation.
			 
			    This silences sounds played when alt + enter was pressed multiple times and some other weird behavior. 

				TODO Maybe a better way how to distinguish when to pass the character to the default handler and when to ignore it based on whether the shortcut is actually used?
			 */
			case WM_SYSCHAR:
			    switch (wParam) {
					case helpers::Char::LF:
					case helpers::Char::CR:
					    return 0;
					default:
					    break;
				}
			    break;
			case WM_CHAR:
				if (wParam >= 0x20)
					window->rendererKeyChar(helpers::Char::FromCodepoint(static_cast<unsigned>(wParam)));
				break;
			/* Processes special key events.
			
			   TODO perhaps all the syskeydown & syskeyup events should be stopped? 
			 */
			case WM_SYSKEYDOWN:
			case WM_KEYDOWN: {
				Key k = GetKey(static_cast<unsigned>(wParam));
				if (k != ui::Key::Invalid)
					window->rendererKeyDown(k);
				// returning w/o calling the default window proc means that the OS will not interfere by interpreting own shortcuts
				// NOTE add other interfering shortcuts as necessary
				if (k == ui::Key::F10 || k.code() == ui::Key::AltKey)
				    return 0;
				break;
			}
			/* The modifier part of the key corresponds to the state of the modifiers *after* the key has been released. 
			 */
			case WM_SYSKEYUP:
			case WM_KEYUP: {
				Key k = GetKey(static_cast<unsigned>(wParam));
				window->rendererKeyUp(k);
				break;
			}


        }
        return DefWindowProc(hWnd, msg, wParam, lParam);
    }


} // namespace tpp

namespace tpp {

	DirectWriteWindow::DirectWriteWindow(std::string const & title, int cols, int rows, unsigned baseCellHeightPx):
	    RendererWindow(cols, rows, DirectWriteFont::GetOrCreate(ui::Font(), 0, baseCellHeightPx)->widthPx(), baseCellHeightPx),
		    wndPlacement_{ sizeof(wndPlacement_) },
			frameWidthPx_(0),
			frameHeightPx_(0),
			font_(nullptr),
			glyphIndices_(nullptr),
			glyphAdvances_(nullptr),
			glyphOffsets_(nullptr),
			mouseLeaveTracked_(false) {
			helpers::utf16_string t = helpers::UTF8toUTF16(title);
			hWnd_ = CreateWindowExW(
				WS_EX_LEFT, // the default
				DirectWriteApplication::Instance()->WindowClassName_, // window class
				// ok, on windows wchar_t and char16_t are the same (see helpers/char.h)
				t.c_str(), // window name (all start as terminal++)
				WS_OVERLAPPEDWINDOW,
				CW_USEDEFAULT, // x position
				CW_USEDEFAULT, // y position
				widthPx_,
				heightPx_,
				nullptr, // handle to parent
				nullptr, // handle to menu 
				DirectWriteApplication::Instance()->hInstance_, // module handle
				this // lParam for WM_CREATE message
			);

			D2D1_SIZE_U size = D2D1::SizeU(widthPx_, heightPx_);
			OSCHECK(SUCCEEDED(DirectWriteApplication::Instance()->d2dFactory_->CreateHwndRenderTarget(
				D2D1::RenderTargetProperties(),
				D2D1::HwndRenderTargetProperties(
					hWnd_,
					size
				),
				&rt_
			)));
			rt_->SetTransform(D2D1::IdentityMatrix());

			OSCHECK(SUCCEEDED(rt_->CreateSolidColorBrush(
				D2D1::ColorF(D2D1::ColorF::White),
				&fg_
			)));
			OSCHECK(SUCCEEDED(rt_->CreateSolidColorBrush(
				D2D1::ColorF(D2D1::ColorF::Black),
				&bg_
			)));
			OSCHECK(SUCCEEDED(rt_->CreateSolidColorBrush(
				D2D1::ColorF(D2D1::ColorF::White),
				&decor_
			)));
			OSCHECK(SUCCEEDED(rt_->CreateSolidColorBrush(
				D2D1::ColorF(0xffffff, 0.5),
				&border_
			)));
			ZeroMemory(&glyphRun_, sizeof(DWRITE_GLYPH_RUN));

			updateDirectWriteStructures(cols_);

			AddWindowNativeHandle(this, hWnd_);

		}


    DirectWriteWindow::~DirectWriteWindow() {
		if (RemoveWindow(hWnd_))
            PostQuitMessage(0);
    }


	/** Basically taken from:

		https://devblogs.microsoft.com/oldnewthing/20100412-00/?p=14353
	 */
    void DirectWriteWindow::updateFullscreen(bool value) {
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
				return;
			}
		} else {
			SetWindowLong(hWnd_, GWL_STYLE, style | WS_OVERLAPPEDWINDOW);
			SetWindowPlacement(hWnd_, &wndPlacement_);
			SetWindowPos(hWnd_, nullptr, 0, 0, 0, 0,
				SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER |
				SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
		}
        // actually update the value
        Window::updateFullscreen(value);
    }

	void DirectWriteWindow::updateZoom(double value) {
		DirectWriteFont * f = DirectWriteFont::GetOrCreate(ui::Font(), 0, static_cast<unsigned>(baseCellHeightPx_ * value));
		cellWidthPx_ = f->widthPx();
		cellHeightPx_ = f->heightPx();
		Super::updateZoom(value);
		Super::updateSizePx(widthPx_, heightPx_);
	}

	void DirectWriteWindow::requestClipboardContents() {
		std::string result;
		if (OpenClipboard(nullptr)) {
			HANDLE clip = GetClipboardData(CF_UNICODETEXT);
			if (clip) {
				// ok, on windows wchar_t and char16_t are the same (see helpers/char.h)
				helpers::utf16_char* data = reinterpret_cast<helpers::utf16_char*>(GlobalLock(clip));
				if (data) {
					result = helpers::UTF16toUTF8(data);
					GlobalUnlock(clip);
				}
			}
			CloseClipboard();
		}
        paste(result);
	}

	void DirectWriteWindow::requestSelectionContents() {
		DirectWriteApplication * app = DirectWriteApplication::Instance();
		if (!app->selection_.empty())
		    paste(app->selection_);
		else
		    paste(std::string{});
	}

    void DirectWriteWindow::setClipboard(std::string const & contents) {
		if (OpenClipboard(nullptr)) {
			EmptyClipboard();
			// encode the string into UTF16 and get the size of the data we need
			// ok, on windows wchar_t and char16_t are the same (see helpers/char.h)
			helpers::utf16_string str = helpers::UTF8toUTF16(contents);
			// the str is null-terminated
			size_t size = (str.size() + 1) * 2;
			HGLOBAL clip = GlobalAlloc(0, size);
			if (clip) {
				WCHAR* data = reinterpret_cast<WCHAR*>(GlobalLock(clip));
				if (data) {
					memcpy(data, str.c_str(), size);
					GlobalUnlock(clip);
					SetClipboardData(CF_UNICODETEXT, clip);
				}
			}
			CloseClipboard();
		}
	}

	void DirectWriteWindow::setSelection(std::string const & contents) {
		DirectWriteApplication * app = DirectWriteApplication::Instance();
		// if the selectionOwner is own window, then there is no need to informa the window of selection change as it has done so already if necessary, in other cases we must inform the old owner that its selection has been invalidated
		if (app->selectionOwner_ != this && app->selectionOwner_ != nullptr)
		    app->selectionOwner_->selectionInvalidated();
		// set the contents and owner
		app->selection_ = contents;
	    app->selectionOwner_ = this;
	}

	void DirectWriteWindow::clearSelection() {
		DirectWriteApplication * app = DirectWriteApplication::Instance();
		if (app->selectionOwner_ == this) {
			app->selectionOwner_ = nullptr;
			app->selection_.clear();
		} else {
			LOG() << "Window renderer clear selection does not match stored selection owner.";
		}
	}

	void DirectWriteWindow::updateDirectWriteStructures(int cols) {
		delete[] glyphIndices_;
		delete[] glyphAdvances_;
		delete[] glyphOffsets_;
		glyphIndices_ = new UINT16[cols];
		glyphAdvances_ = new FLOAT[cols];
		glyphOffsets_ = new DWRITE_GLYPH_OFFSET[cols];
		glyphRun_.glyphIndices = glyphIndices_;
		glyphRun_.glyphAdvances = glyphAdvances_;
		glyphRun_.glyphOffsets = glyphOffsets_;
		ZeroMemory(glyphOffsets_, sizeof(DWRITE_GLYPH_OFFSET) * cols);
		// nothing to be done with advances and indices as these are filled by the drawing method 
		glyphRun_.glyphCount = 0;
	}

	// https://docs.microsoft.com/en-us/windows/desktop/inputdev/virtual-key-codes
	ui::Key DirectWriteWindow::GetKey(unsigned vk) {
		// we don't distinguish between left and right win keys
		if (vk == VK_RWIN)
			vk = VK_LWIN;
		if (! ui::Key::IsValidCode(vk))
			return ui::Key(ui::Key::Invalid);
		// MSB == pressed, LSB state since last time
		unsigned shift = (GetAsyncKeyState(VK_SHIFT) & 0x8000) ? ui::Key::Shift : 0;
		unsigned ctrl = (GetAsyncKeyState(VK_CONTROL) & 0x8000) ? ui::Key::Ctrl : 0;
		unsigned alt = (GetAsyncKeyState(VK_MENU) & 0x8000) ? ui::Key::Alt : 0;
		unsigned win = (GetAsyncKeyState(VK_LWIN) & 0x8000) || (GetAsyncKeyState(VK_RWIN) & 0x8000) ? ui::Key::Win : 0;

		return ui::Key(vk, shift | ctrl | alt | win);
	}

	LRESULT CALLBACK DirectWriteWindow::EventHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        // obtain the window object (nullptr if unknown)
        DirectWriteWindow * window = GetWindowFromNativeHandle(hWnd);
        switch (msg) {
			/** Closes the current window. */
			case WM_CLOSE: {
				ASSERT(window != nullptr) << "Unknown window";
				break;
			}
			/** Destroys the window, if it is the last window, quits the app. */
			case WM_DESTROY: {
				ASSERT(window != nullptr) << "Attempt to destroy unknown window";
				// delete the window object
                delete window;
				break;
			}
			/* When the window is created, the border width and height of a terminal window is determined and the window's size is updated to adjust for it. */
			case WM_CREATE: {
				// now calculate the border and actually update the window size to account for it
				CREATESTRUCT& cs = *reinterpret_cast<CREATESTRUCT*>(lParam);
				// get the tw member from the create struct argument
				ASSERT(window == nullptr);
				window = reinterpret_cast<DirectWriteWindow*>(cs.lpCreateParams);
				ASSERT(window != nullptr);
				RECT r;
				r.left = cs.x;
				r.right = cs.x + cs.cx;
				r.top = cs.y;
				r.bottom = cs.y + cs.cy;
				AdjustWindowRectEx(&r, cs.style, false, cs.dwExStyle);
				unsigned fw = r.right - r.left - cs.cx;
				unsigned fh = r.bottom - r.top - cs.cy;
				if (fw != 0 || fh != 0) {
					window->frameWidthPx_ = fw;
					window->frameHeightPx_ = fh;
					SetWindowPos(hWnd, HWND_TOP, cs.x, cs.y, cs.cx + fw, cs.cy + fh, SWP_NOZORDER | SWP_NOACTIVATE);
				}
				break;
			}
			/** Window gains focus. 
			 */
			case WM_SETFOCUS:
				if (window != nullptr)
    				window->setFocus(true);
				break;
			/** Window loses focus. 
			 */
			case WM_KILLFOCUS:
				if (window != nullptr)
    				window->setFocus(false);
				break;
			/* Called when the window is resized interactively by the user. Makes sure that the window size snaps to discrete terminal sizes. */
			case WM_SIZING: {
				RECT* winRect = reinterpret_cast<RECT*>(lParam);
				switch (wParam) {
                    case WMSZ_BOTTOM:
                    case WMSZ_BOTTOMRIGHT:
                    case WMSZ_BOTTOMLEFT:
                        winRect->bottom -= (winRect->bottom - winRect->top - window->frameHeightPx_) % window->cellHeightPx_;
                        break;
                    default:
                        winRect->top += (winRect->bottom - winRect->top - window->frameHeightPx_) % window->cellHeightPx_;
                        break;
				}
				switch (wParam) {
                    case WMSZ_RIGHT:
                    case WMSZ_TOPRIGHT:
                    case WMSZ_BOTTOMRIGHT:
                        winRect->right -= (winRect->right - winRect->left - window->frameWidthPx_) % window->cellWidthPx_;
                        break;
                    default:
                        winRect->left += (winRect->right - winRect->left - window->frameWidthPx_) % window->cellWidthPx_;
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
				if (window != nullptr) {
					RECT rect;
					GetClientRect(hWnd, &rect);
					window->updateSizePx(rect.right, rect.bottom);
				}
				break;
			}
			/* Repaint of the window is requested. */
			case WM_PAINT: {
				ASSERT(window != nullptr) << "Attempt to paint unknown window";
				window->paint();
				break;
			}
			/* No need to use WM_UNICHAR since WM_CHAR is already unicode aware */
			case WM_UNICHAR:
				UNREACHABLE;
				break;
			/* If a system character is intercepted, do nothing & bypass the default implementation.
			 
			    This silences sounds played when alt + enter was pressed multiple times and some other weird behavior. 

				TODO Maybe a better way how to distinguish when to pass the character to the default handler and when to ignore it based on whether the shortcut is actually used?
			 */
			case WM_SYSCHAR:
			    switch (wParam) {
					case helpers::Char::LF:
					case helpers::Char::CR:
					    return 0;
					default:
					    break;
				}
			    break;
			case WM_CHAR:
				if (wParam >= 0x20)
					window->keyChar(helpers::Char::FromCodepoint(static_cast<unsigned>(wParam)));
				break;
			/* Processes special key events.
			
			   TODO perhaps all the syskeydown & syskeyup events should be stopped? 
			 */
			case WM_SYSKEYDOWN:
			case WM_KEYDOWN: {
				ui::Key k = GetKey(static_cast<unsigned>(wParam));
				if (k != ui::Key::Invalid)
					window->keyDown(k);
				// returning w/o calling the default window proc means that the OS will not interfere by interpreting own shortcuts
				// NOTE add other interfering shortcuts as necessary
				if (k == ui::Key::F10 || k.code() == ui::Key::AltKey)
				    return 0;
				break;
			}
			/* The modifier part of the key corresponds to the state of the modifiers *after* the key has been released. 
			 */
			case WM_SYSKEYUP:
			case WM_KEYUP: {
				ui::Key k = GetKey(static_cast<unsigned>(wParam));
				window->keyUp(k);
				break;
			}
			/* Mouse events which simply obtain the mouse coordinates, convert the buttons and wheel values to vterm standards and then calls the DirectWriteTerminalWindow's events, which perform the pixels to cols & rows translation and then call the terminal itself.
			 */
#define MOUSE_X static_cast<unsigned>(lParam & 0xffff)
#define MOUSE_Y static_cast<unsigned>((lParam >> 16) & 0xffff)
			case WM_LBUTTONDOWN:
				window->mouseDown(MOUSE_X, MOUSE_Y, ui::MouseButton::Left);
				break;
			case WM_LBUTTONUP:
				window->mouseUp(MOUSE_X, MOUSE_Y, ui::MouseButton::Left);
				break;
			case WM_RBUTTONDOWN:
				window->mouseDown(MOUSE_X, MOUSE_Y, ui::MouseButton::Right);
				break;
			case WM_RBUTTONUP:
				window->mouseUp(MOUSE_X, MOUSE_Y, ui::MouseButton::Right);
				break;
			case WM_MBUTTONDOWN:
				window->mouseDown(MOUSE_X, MOUSE_Y, ui::MouseButton::Wheel);
				break;
			case WM_MBUTTONUP:
				window->mouseUp(MOUSE_X, MOUSE_Y, ui::MouseButton::Wheel);
				break;
			/* Mouse wheel contains the position relative to screen top/left, so we must first translate it to window coordinates. 
			 */
			case WM_MOUSEWHEEL: {
				POINT pos{ MOUSE_X, MOUSE_Y };
				ScreenToClient(hWnd, &pos);
				window->mouseWheel(pos.x, pos.y, GET_WHEEL_DELTA_WPARAM(wParam) / WHEEL_DELTA);
				break;
			}
			case WM_MOUSEMOVE:
				window->mouseMove(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
				break;
			/* Triggered when mouse leaves the window (the mouse leave tracking was previously turned on in mouseMove window method) 
 			 */
			case WM_MOUSELEAVE:
			    //ASSERT(window->mouseLeaveTracked_);
				window->mouseLeaveTracked_ = false;
				window->mouseLeave();
				break;
			/** Send when mouse capture has been lost (either explicitly, or implicitly). 

			    Does nothing and just exists there as a simple placeholder if in future the capture change should be reflected.
			 */
			case WM_CAPTURECHANGED:
			    break;
			/* 
			 */
			case WM_USER:
			    switch (wParam) {
					case DirectWriteApplication::MSG_TITLE_CHANGE: {
						helpers::utf16_string t = helpers::UTF8toUTF16(window->rootWindow()->title());
						// ok, on windows wchar_t and char16_t are the same (see helpers/char.h)
						SetWindowTextW(hWnd, t.c_str());
						break;
					}					
				    default:
					    LOG() << "Invalid user message " << wParam;
				}
				break;
        }
        return DefWindowProc(hWnd, msg, wParam, lParam);
    }

} // namespace tpp

#endif

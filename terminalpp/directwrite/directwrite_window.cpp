#if (defined ARCH_WINDOWS && defined RENDERER_NATIVE)
#include "directwrite_window.h"

#include "directwrite_font.h"

namespace tpp {

    void DirectWriteWindow::setTitle(std::string const & value) {
        RendererWindow::setTitle(value);
        // actually change the title since we are in the UI thread now
        utf16_string t = UTF8toUTF16(value);
        // ok, on windows wchar_t and char16_t are the same (see helpers/char.h)
        SetWindowTextW(hWnd_, t.c_str());
    }

    void DirectWriteWindow::setIcon(Window::Icon icon) {
        RendererWindow::setIcon(icon);
        DirectWriteApplication *app = DirectWriteApplication::Instance();
        WPARAM iconHandle;
        switch (icon) {
            case Icon::Notification:
                iconHandle = reinterpret_cast<WPARAM>(app->iconNotification_);
                break;
            default:
                iconHandle = reinterpret_cast<WPARAM>(app->iconDefault_);
                break;
        }
        PostMessage(hWnd_, WM_SETICON, ICON_BIG, iconHandle);
        PostMessage(hWnd_, WM_SETICON, ICON_SMALL, iconHandle);
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

    void DirectWriteWindow::schedule(std::function<void()> event, Widget * widget) {
        EventQueue::schedule(event, widget);
        PostMessage(DirectWriteApplication::Instance()->dummy_, WM_USER, 0, 0);
    }

    void DirectWriteWindow::requestClipboard(Widget * sender) {
        RendererWindow::requestClipboard(sender);
		std::string result;
		if (OpenClipboard(nullptr)) {
			HANDLE clip = GetClipboardData(CF_UNICODETEXT);
			if (clip) {
				// ok, on windows wchar_t and char16_t are the same (see helpers/char.h)
				utf16_char* data = reinterpret_cast<utf16_char*>(GlobalLock(clip));
				if (data) {
					result = UTF16toUTF8(data);
					GlobalUnlock(clip);
				}
			}
			CloseClipboard();
		}
        pasteClipboard(result);
    }

    void DirectWriteWindow::requestSelection(Widget * sender) {
        RendererWindow::requestSelection(sender);
        DirectWriteApplication * app = DirectWriteApplication::Instance();
        if (app->selectionOwner_ != nullptr)
            pasteSelection(app->selection_);
    }

    void DirectWriteWindow::setClipboard(std::string const & contents) {
        DirectWriteApplication::Instance()->setClipboard(contents);
    }

    void DirectWriteWindow::setSelection(std::string const & contents, Widget * owner) {
        DirectWriteApplication * app = DirectWriteApplication::Instance();
        // first we clear the selection if it belongs to different window
        if (app->selectionOwner_ != nullptr && app->selectionOwner_ != this)
            app->selectionOwner_->clearSelection(nullptr);
        // set the contents and the selection owner to ourselves
        app->selectionOwner_ = this;
        app->selection_ = contents;
    }

    void DirectWriteWindow::clearSelection(Widget * sender) {
        // on windows, this is rather simple, first deal with the application global selection, then call the parent implementation
        DirectWriteApplication * app = DirectWriteApplication::Instance();
        if (app->selectionOwner_ == this) {
            app->selectionOwner_ = nullptr;
            app->selection_.clear();
        }
        RendererWindow::clearSelection(sender);
    }

    DirectWriteWindow::DirectWriteWindow(std::string const & title, int cols, int rows):
        RendererWindow<DirectWriteWindow, HWND>{cols, rows},
        wndPlacement_{ sizeof(wndPlacement_) },
        frameWidth_{0},
        frameHeight_{0},
        font_(nullptr),
        glyphIndices_{nullptr},
        glyphAdvances_{nullptr},
        glyphOffsets_{nullptr},
        mouseLeaveTracked_{false} {
        // create the window with given title
        utf16_string t = UTF8toUTF16(title);
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
        D2D1_SIZE_U dxsize = D2D1::SizeU(widthPx_, heightPx_);
        OSCHECK(SUCCEEDED(DirectWriteApplication::Instance()->d2dFactory_->CreateHwndRenderTarget(
            D2D1::RenderTargetProperties(),
            D2D1::HwndRenderTargetProperties(
                hWnd_,
                dxsize
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

        // create the glyph run buffer and size it appropriately
        updateDirectWriteStructures(size().width());

        // register the window
        RegisterWindowHandle(this, hWnd_);  
        
        setTitle(title_);
        setIcon(icon_);      
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
	Key DirectWriteWindow::GetKey(unsigned vk) {
		// we don't distinguish between left and right win keys
		if (vk == VK_RWIN)
			vk = VK_LWIN;
        Key result = Key::FromCode(vk);
        if (result == Key::Invalid)
            return result;
		// MSB == pressed, LSB state since last time
        if (GetAsyncKeyState(VK_SHIFT) & 0x8000)
            result = result + Key::Shift;
        if (GetAsyncKeyState(VK_CONTROL) & 0x8000)
            result = result + Key::Ctrl;
        if (GetAsyncKeyState(VK_MENU) & 0x8000)
            result = result + Key::Alt;
        if ((GetAsyncKeyState(VK_LWIN) & 0x8000) || (GetAsyncKeyState(VK_RWIN) & 0x8000))
            result = result + Key::Win;
        return result;
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
			/** Closes the current window. 
             
                Calls the renderer's requestClose() method which shoudl in call renderer's closing function that calls DestroyWindow. 
             */
			case WM_CLOSE: {
				ASSERT(window != nullptr) << "Unknown window";
                // TODO determine how to deal with closing the window !!!!!
                //window->requestClose();
                return 0;
			}
			/** Destroys the window, if it is the last window, quits the app. */
			case WM_DESTROY: {
				ASSERT(window != nullptr) << "Attempt to destroy unknown window";
				// delete the window object
                delete window;
                if (Windows().empty())
                    PostQuitMessage(EXIT_SUCCESS);
				break;
			}
            /** Window gains keyboard focus. 
			 */
			case WM_SETFOCUS:
				if (window != nullptr)
    				window->focusIn();
				break;
			/** Window loses keyboard focus. 
			 */
			case WM_KILLFOCUS:
				if (window != nullptr)
    				window->focusOut();
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
                window->repaint();
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
					case Char::LF:
					case Char::CR:
					    return 0;
					default:
					    break;
				}
			    break;
			case WM_CHAR:
				if (wParam >= 0x20)
					window->keyChar(Char{static_cast<char32_t>(wParam)});
				break;
			/* Processes special key events.
			
			   TODO perhaps all the syskeydown & syskeyup events should be stopped? 
			 */
			case WM_SYSKEYDOWN:
			case WM_KEYDOWN: {
				Key k = GetKey(static_cast<unsigned>(wParam));
				if (k != Key::Invalid)
					window->keyDown(k);
				// returning w/o calling the default window proc means that the OS will not interfere by interpreting own shortcuts
				// NOTE add other interfering shortcuts as necessary
				if (k == Key::F10 || k.code() == Key::AltKey)
				    return 0;
				break;
			}
			/* The modifier part of the key corresponds to the state of the modifiers *after* the key has been released. 
			 */
			case WM_SYSKEYUP:
			case WM_KEYUP: {
				Key k = GetKey(static_cast<unsigned>(wParam));
				window->keyUp(k);
				break;
			}
			/* Mouse events which simply obtain the mouse coordinates, convert the buttons and wheel values to vterm standards and then calls the DirectWriteTerminalWindow's events, which perform the pixels to cols & rows translation and then call the terminal itself.
			 */
#define MOUSE_X static_cast<unsigned>(lParam & 0xffff)
#define MOUSE_Y static_cast<unsigned>((lParam >> 16) & 0xffff)
			case WM_LBUTTONDOWN:
				window->mouseDown(window->pixelsToCoords(Point{MOUSE_X, MOUSE_Y}), MouseButton::Left);
				break;
			case WM_LBUTTONUP:
				window->mouseUp(window->pixelsToCoords(Point{MOUSE_X, MOUSE_Y}), MouseButton::Left);
				break;
			case WM_RBUTTONDOWN:
				window->mouseDown(window->pixelsToCoords(Point{MOUSE_X, MOUSE_Y}), MouseButton::Right);
				break;
			case WM_RBUTTONUP:
				window->mouseUp(window->pixelsToCoords(Point{MOUSE_X, MOUSE_Y}), MouseButton::Right);
				break;
			case WM_MBUTTONDOWN:
				window->mouseDown(window->pixelsToCoords(Point{MOUSE_X, MOUSE_Y}), MouseButton::Wheel);
				break;
			case WM_MBUTTONUP:
				window->mouseUp(window->pixelsToCoords(Point{MOUSE_X, MOUSE_Y}), MouseButton::Wheel);
				break;
			/* Mouse wheel contains the position relative to screen top/left, so we must first translate it to window coordinates. 
			 */
			case WM_MOUSEWHEEL: {
				POINT pos{ MOUSE_X, MOUSE_Y };
				ScreenToClient(hWnd, &pos);
				window->mouseWheel(window->pixelsToCoords(Point{pos.x, pos.y}), GET_WHEEL_DELTA_WPARAM(wParam) / WHEEL_DELTA);
				break;
			}
			case WM_MOUSEMOVE:
				window->mouseMove(window->pixelsToCoords(Point{GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)}));
				break;
			/* Triggered when mouse leaves the window.
 			 */
			case WM_MOUSELEAVE:
				window->mouseLeaveTracked_ = false;
				window->mouseOut();
				break;

        }
        return DefWindowProc(hWnd, msg, wParam, lParam);
    }


} // namespace tpp

#endif

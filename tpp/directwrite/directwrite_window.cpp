#if (defined ARCH_WINDOWS)
#include "directwrite_window.h"

#include "directwrite_font.h"

namespace tpp {

    std::unordered_map<HWND, DirectWriteWindow*> DirectWriteWindow::Windows_;

	DirectWriteWindow::DirectWriteWindow(std::string const & title, int cols, int rows, unsigned baseCellHeightPx):
	    RendererWindow(title, cols, rows, Font::GetOrCreate(ui::Font(), baseCellHeightPx)->cellWidthPx(), baseCellHeightPx),
		    wndPlacement_{ sizeof(wndPlacement_) },
			frameWidthPx_(0),
			frameHeightPx_(0),
			dwFont_(nullptr),
			glyphIndices_(nullptr),
			glyphAdvances_(nullptr),
			glyphOffsets_(nullptr) {
			helpers::utf16_string t = helpers::UTF8toUTF16(title_);
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
				D2D1::ColorF(D2D1::ColorF::Black),
				&decor_
			)));
			ZeroMemory(&glyphRun_, sizeof(DWRITE_GLYPH_RUN));

			updateDirectWriteStructures(cols_);


			Windows_.insert(std::make_pair(hWnd_, this));
		}


    DirectWriteWindow::~DirectWriteWindow() {
        Windows_.erase(hWnd_);
        if (Windows_.empty())
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
		Font * f = Font::GetOrCreate(ui::Font(), static_cast<unsigned>(baseCellHeightPx_ * value));
		cellWidthPx_ = f->cellWidthPx();
		cellHeightPx_ = f->cellHeightPx();
		Window::updateZoom(value);
		Window::updateSize(widthPx_, heightPx_);
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
        DirectWriteWindow * window = GetWindowFromHWND(hWnd);
        switch (msg) {
			/** Closes the current window. */
			case WM_CLOSE: {
				ASSERT(window != nullptr) << "Unknown window";
				//Session::Close(tw->session());
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
				window->render();
				break;
			}
			/* No need to use WM_UNICHAR since WM_CHAR is already unicode aware */
			case WM_UNICHAR:
				UNREACHABLE;
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
				window->mouseMove(MOUSE_X, MOUSE_Y);
				break;


        }
        return DefWindowProc(hWnd, msg, wParam, lParam);
    }

} // namespace tpp

#endif

#include "application.h"

#include "terminal_window.h"



namespace tpp {

	std::unordered_map<HWND, TerminalWindow *> TerminalWindow::Windows_;

	TerminalWindow::TerminalWindow(Application * app, TerminalSettings * settings):
	    BaseTerminalWindow(settings) {
		hWnd_ = CreateWindowEx(
			WS_EX_LEFT, // the default
			app->TerminalWindowClassName_, // window class
			name_.c_str(), // window name (all start as terminal++)
			WS_OVERLAPPEDWINDOW,
			CW_USEDEFAULT, // x position
			CW_USEDEFAULT, // y position
			800,
			600,
			nullptr, // handle to parent
			nullptr, // handle to menu - TODO I should actually create a menu
			app->hInstance_, // module handle
			this // lParam for WM_CREATE message
		);
		ASSERT(hWnd_ != 0) << "Cannot create window : " << GetLastError();
		Windows_.insert(std::make_pair(hWnd_, this));
	}

	TerminalWindow::~TerminalWindow() {

	}

	void TerminalWindow::repaint(vterm::Terminal::RepaintEvent & e) {

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

				/*
				CREATESTRUCT & cs = *reinterpret_cast<CREATESTRUCT*>(lParam);
				RECT r;
				r.left = cs.x;
				r.right = cs.x + cs.cx;
				r.top = cs.y;
				r.bottom = cs.y + cs.cy;
				AdjustWindowRectEx(&r, cs.style, false, cs.dwExStyle);
				unsigned bw = r.right - r.left - cs.cx;
				unsigned bh = r.bottom - r.top - cs.cy;
				if (bw != 0 || bh != 0) {
					BorderWidth_ = bw;
					BorderHeight_ = bh;
					SetWindowPos(hWnd, HWND_TOP, cs.x, cs.y, cs.cx + bw, cs.cy + bh, SWP_NOZORDER);
				} */
				break;
			}
		} // end of switch
		return DefWindowProc(hWnd, msg, wParam, lParam);
	}

} // namespace tpp
#pragma once
#ifdef WIN32

#include <unordered_map>

#include <windows.h>
#include <d2d1.h>
#include <dwrite.h>

#include "../base_terminal_window.h"

namespace tpp {

	class Application;

	class TerminalWindow : public BaseTerminalWindow {
	public:
		TerminalWindow(Application * app, TerminalSettings * settings);

		void show() override {
			ShowWindow(hWnd_, SW_SHOWNORMAL);
		}

		void hide() override {
			NOT_IMPLEMENTED;
		}

	protected:

		/** Just deleting a terminal window is not allowed, therefore protected.
		 */
		~TerminalWindow() override;

		void repaint(vterm::Terminal::RepaintEvent & e) override;


	private:
		friend class Application;

		static LRESULT CALLBACK EventHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

		HWND hWnd_;

		static std::unordered_map<HWND, TerminalWindow *> Windows_;

	};

} // namespace tpp
#endif
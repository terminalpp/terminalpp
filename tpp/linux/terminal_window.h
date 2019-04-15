#pragma once
#ifdef __linux__

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <X11/Xft/Xft.h>


#include "../base_terminal_window.h"

namespace tpp {

	class Application;

	class TerminalWindow : public BaseTerminalWindow {
	public:

		TerminalWindow(Application* app, TerminalSettings* settings);


		void show() override;

		void hide() override {
			NOT_IMPLEMENTED;
		}

		void redraw() override {
			NOT_IMPLEMENTED;
		}

	protected:

		~TerminalWindow() override;

		void resizeWindow(unsigned width, unsigned height) override;

		void repaint(vterm::Terminal::RepaintEvent& e) override;

		void doSetFullscreen(bool value) override;

		void doTitleChange(vterm::VT100::TitleEvent& e) override;

		Window window_;
		Display* display_;
		int screen_;
		Visual* visual_;
		Colormap colorMap_;

	}; // TerminalWinfdow [linux]

} // namespace tpp
#endif 
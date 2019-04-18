#pragma once
#ifdef __linux__

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <X11/Xft/Xft.h>


#include "../base_terminal_window.h"

#include "application.h"

namespace tpp {


	template<>
	inline FontSpec<XftFont*>* FontSpec<XftFont*>::Create(vterm::Font font, unsigned height) {
		//std::string fname = STR("Iosevka Term:pixelsize=" << height);
		std::string fname = STR("Iosevka Term:pixelsize=" << (height - 3));
		if (font.bold())
			fname += ":bold";
		if (font.italics())
			fname += ":italic";
		// TODO underline and strikethrough

		XGlyphInfo gi;
		XftFont* handle = XftFontOpenName(Application::XDisplay(), Application::XScreen(), fname.c_str());
        ASSERT(handle != nullptr);
		XftTextExtentsUtf8(Application::XDisplay(), handle, (FcChar8*)"m", 1, &gi);
		LOG << handle->ascent;
		LOG << handle->descent;
		return new FontSpec<XftFont*>(font, gi.width, handle->ascent + handle->descent, handle);
	}

	class TerminalWindow : public BaseTerminalWindow {
	public:

		typedef FontSpec<XftFont*> Font;

		TerminalWindow(Application* app, TerminalSettings* settings);


		void show() override;

		void hide() override {
			NOT_IMPLEMENTED;
		}

		void redraw() override;

	protected:

		~TerminalWindow() override;

		void resizeWindow(unsigned width, unsigned height) override;

		void repaint(vterm::Terminal::RepaintEvent& e) override;

		void doSetFullscreen(bool value) override;

		void doTitleChange(vterm::VT100::TitleEvent& e) override;

		XftColor toXftColor(vterm::Color const& c) {
			XftColor result;
			result.color.red = c.red * 256;
			result.color.green = c.green * 256;
			result.color.blue = c.blue * 256;
			result.color.alpha = 65535;
			return result;
		}

	private:

        friend class Application;


		void updateBuffer();

        static void EventHandler(XEvent & e);

		Window window_;
		Display* display_;
		int screen_;
		Visual* visual_;
		Colormap colorMap_;

		static std::unordered_map<Window, TerminalWindow *> Windows_;

	}; // TerminalWinfdow [linux]

} // namespace tpp
#endif 
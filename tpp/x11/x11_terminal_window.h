#pragma once
#ifdef __linux__

#include <mutex>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <X11/Xft/Xft.h>


#include "../terminal_window.h"

#include "x11_application.h"

namespace tpp {


	template<>
	inline FontSpec<XftFont*>* FontSpec<XftFont*>::Create(vterm::Font font, unsigned height) {
		X11Application* app = reinterpret_cast<X11Application*>(Application::Instance());
		//std::string fname = STR("Iosevka Term:pixelsize=" << height);
		std::string fname = STR("Iosevka Term:pixelsize=" << (height - 3));
		if (font.bold())
			fname += ":bold";
		if (font.italics())
			fname += ":italic";
		// TODO underline and strikethrough

		XGlyphInfo gi;
		XftFont* handle = XftFontOpenName(app->xDisplay(), app->xScreen(), fname.c_str());
        ASSERT(handle != nullptr);
		XftTextExtentsUtf8(app->xDisplay(), handle, (FcChar8*)"m", 1, &gi);
		return new FontSpec<XftFont*>(font, gi.width, handle->ascent + handle->descent, handle);
	}

	class X11TerminalWindow : public TerminalWindow {
	public:

		typedef FontSpec<XftFont*> Font;

		X11TerminalWindow(Properties const & properties, std::string const & title);


		void show() override;

		void hide() override {
			NOT_IMPLEMENTED;
		}

	protected:

		/** Returns the application instance casted to X11 app. 
		 */
		X11Application* app() {
			return reinterpret_cast<X11Application*>(Application::Instance());
		}

		~X11TerminalWindow() override;

		void doSetFullscreen(bool value) override;

		void titleChange(vterm::Terminal::TitleChangeEvent & e) override;

		void doInvalidate() override {
            // set the flag
            TerminalWindow::doInvalidate(); 
            // trigger a refresh
            XExposeEvent msg;
            memset(&msg, 0, sizeof(XExposeEvent));
            msg.type = Expose;
            msg.display = display_;
            msg.window = window_;
            XSendEvent(display_, window_, false, ExposureMask, (XEvent *) & msg);
            XFlush(display_);
		}

		void clipboardPaste() override;
		void clipboardCopy(std::string const& str) override;

		void doPaint() override;

		void doSetForeground(vterm::Color const& fg) override {
			fg_ = toXftColor(fg);
		}

		void doSetBackground(vterm::Color const& bg) override {
			bg_ = toXftColor(bg);
		}

		void doSetFont(vterm::Font font) override {
			font_ = Font::GetOrCreate(font, cellHeightPx_);
		}

		void doDrawCell(unsigned col, unsigned row, vterm::Terminal::Cell const& c) override {
			XftDrawRect(draw_, &bg_, col * cellWidthPx_, row * cellHeightPx_, cellWidthPx_, cellHeightPx_);
			XftDrawStringUtf8(draw_, &fg_, font_->handle(), col * cellWidthPx_, (row + 1) * cellHeightPx_ - font_->handle()->descent, (XftChar8*)(c.c.rawBytes()), c.c.size());
		}

		void doDrawCursor(unsigned col, unsigned row, vterm::Terminal::Cell const& c) override {
			XftDrawStringUtf8(draw_, &fg_, font_->handle(), col * cellWidthPx_, (row + 1) * cellHeightPx_ - font_->handle()->descent, (XftChar8*)(c.c.rawBytes()), c.c.size());
		}

		XftColor toXftColor(vterm::Color const& c) {
			XftColor result;
			result.color.red = c.red * 256;
			result.color.green = c.green * 256;
			result.color.blue = c.blue * 256;
			result.color.alpha = 65535;
			return result;
		}

	private:

        friend class X11Application;


		//void updateBuffer();

        /** Converts the KeySym and state as reported by X11 to vterm's Key. 
         */
        static vterm::Key GetKey(KeySym k, unsigned state);

        static void EventHandler(XEvent & e);

		Window window_;
		Display* display_;
		int screen_;
		Visual* visual_;
		Colormap colorMap_;
        XIC ic_;

        GC gc_;
        Pixmap buffer_;

		XftDraw * draw_;
		XftColor fg_;
		XftColor bg_;
		Font * font_;

        std::mutex drawGuard_;
        std::atomic<bool> invalidate_;

		static std::unordered_map<Window, X11TerminalWindow *> Windows_;

	}; // TerminalWinfdow [linux]

} // namespace tpp
#endif 
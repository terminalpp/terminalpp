#pragma once
#ifdef __linux__

#include <mutex>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <X11/Xft/Xft.h>


#include "../config.h"
#include "../terminal_window.h"

#include "x11_application.h"

namespace tpp {


	class X11TerminalWindow : public TerminalWindow {
	public:

		typedef FontSpec<XftFont*> Font;

		X11TerminalWindow(Session * session, Properties const & properties, std::string const & title);


		void show() override;

		void hide() override {
			NOT_IMPLEMENTED;
		}

		void close() override {
			XDestroyWindow(display_, window_);
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

		void clipboardUpdated(vterm::Terminal::ClipboardUpdateEvent& e) override;

		void windowResized(unsigned widthPx, unsigned heightPx) {
			if (buffer_ != 0) {
				XFreePixmap(display_, buffer_);
				buffer_ = 0;
			}
            updateTextStructures(widthPx, cellWidthPx_);
			TerminalWindow::windowResized(widthPx, heightPx);
		}

        void doSetZoom(double value) override {
            TerminalWindow::doSetZoom(value);
            updateTextStructures(widthPx_, cellWidthPx_);
        }

        void updateTextStructures(unsigned width, unsigned fontWidth) {
            delete text_;
            text_ = new XftCharSpec[width / fontWidth];
        }

		void doInvalidate() override {
            // set the flag
            TerminalWindow::doInvalidate(); 
            // trigger a refresh
            XEvent e;
            memset(&e, 0, sizeof(XEvent));
            e.xexpose.type = Expose;
            e.xexpose.display = display_;
            e.xexpose.window = window_;
            app()->xSendEvent(this, e, ExposureMask);
		}

		void clipboardPaste() override;

		unsigned doPaint() override;

		void doSetForeground(vterm::Color const& fg) override {
            drawText();
			fg_ = toXftColor(fg);
		}

		void doSetBackground(vterm::Color const& bg) override {
            drawText();
			bg_ = toXftColor(bg);
		}

		void doSetFont(vterm::Font font) override {
            drawText();
			font_ = Font::GetOrCreate(font, cellHeightPx_);
            textBlink_ = font.blink();
            textUnderline_ = font.underline();
            textStrikethrough_ = font.strikethrough();
		}

		void doDrawCell(unsigned col, unsigned row, vterm::Terminal::Cell const& c) override {
            if (textSize_ != 0 && (col != textCol_ + textSize_ || row != textRow_))
                drawText();
            if (textSize_ == 0) {
                textCol_ = col;
                textRow_ = row;
                text_[0].x = col * cellWidthPx_;
                text_[0].y = row * cellHeightPx_ + font_->handle()->ascent;
            } else {
                text_[textSize_].x = text_[textSize_ - 1].x + cellWidthPx_;
                text_[textSize_].y = text_[textSize_ - 1].y;
            }
            text_[textSize_].ucs4 = c.c().codepoint();
            ++textSize_;
		}

		void doDrawCursor(unsigned col, unsigned row, vterm::Terminal::Cell const& c) override {
			fg_ = toXftColor(c.fg());
			XftDrawStringUtf8(draw_, &fg_, font_->handle(), col * cellWidthPx_, row * cellHeightPx_ + font_->handle()->ascent, (XftChar8*)(c.c().toCharPtr()), c.c().size());
		}

        void drawText() {
            // TODO draw larger rectangle if we are at the end of the screen
            if (textSize_ == 0)
                return;
			XftDrawRect(draw_, &bg_, textCol_ * cellWidthPx_, textRow_ * cellHeightPx_, textSize_ * cellWidthPx_, cellHeightPx_);
			// if the cell is blinking, only draw the text if blink is on
			if (!textBlink_ || blink_) {
                XftDrawCharSpec(draw_, &fg_, font_->handle(), text_, textSize_);
				// renders underline and strikethrough lines
				// TODO for now, this is just approximate values of just below and 2/3 of the font, which is blatantly copied from st and is most likely not typographically correct (see issue 12)
				if (textUnderline_)
					XftDrawRect(draw_, &fg_,textCol_ * cellWidthPx_, textRow_ * cellHeightPx_ + font_->handle()->ascent + 1, cellWidthPx_ * textSize_, 1);
				if (textStrikethrough_)
					XftDrawRect(draw_, &fg_, textCol_ * cellWidthPx_, textRow_ * cellHeightPx_ + (2 * font_->handle()->ascent / 3), cellWidthPx_ * textSize_, 1);
			}
            textSize_ = 0;
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

		class MotifHints {
		public:
			unsigned long   flags;
			unsigned long   functions;
			unsigned long   decorations;
			long            inputMode;
			unsigned long   status;
		};


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


        XftCharSpec * text_;

        /** Text buffer rendering data.
         */
        unsigned textCol_;
        unsigned textRow_;
        unsigned textSize_;
        bool textBlink_;
        bool textUnderline_;
        bool textStrikethrough_;


        std::mutex drawGuard_;
        std::atomic<bool> invalidate_;

		/** Info about the window state before fullscreen was triggered. 
		 */
        XWindowChanges fullscreenRestore_;

		static std::unordered_map<Window, X11TerminalWindow *> Windows_;

		static void FPSTimer() {
			for (auto i : Windows_)
				i.second->fpsTimer();
		}


	}; // TerminalWinfdow [linux]

} // namespace tpp
#endif 
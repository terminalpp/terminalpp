#pragma once
#ifdef ARCH_UNIX

#include <mutex>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <X11/Xatom.h>
#include <X11/Xft/Xft.h>


#include "../config.h"
#include "../terminal_window.h"

#include "x11_application.h"

namespace tpp {


	/**

	    Note that due to the fact that all input events contain the input state, we just update the state modifiers right there when the input happens instead of updating it when focus is set back to the window as does the Windows version. 
	 */
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

		void clipboardUpdate(vterm::Terminal::ClipboardUpdateEvent& e) override;

		void windowResized(unsigned widthPx, unsigned heightPx) override {
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

		void selectionClear(bool manual = true) override;
        void selectionSet() override;

        bool selectionPaste() override;

        /** Pastes the selection's contents into the window.
         */
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
			drawText();
            XftColor cColor = toXftColor(c.fg());
			XftDrawStringUtf8(draw_, &cColor, font_->handle(), col * cellWidthPx_, row * cellHeightPx_ + font_->handle()->ascent, (XftChar8*)(c.c().toCharPtr()), c.c().size());
		}

        void drawText() {
            if (textSize_ == 0)
                return;
			// if we are drawing the last col, or row, clear remaining border as well
			if (textCol_ + textSize_ == cols() || textRow_ == rows() - 1) {
				unsigned clearW = (textCol_ + textSize_ == cols()) ? (widthPx_ - (textCol_ * cellWidthPx_)) : (textSize_ * cellWidthPx_);
				unsigned clearH = (textRow_ == rows() - 1) ? (heightPx_ - (textRow_ * cellHeightPx_)) : (cellHeightPx_);
				XftColor clearC = toXftColor(terminal()->defaultBackgroundColor());
				XftDrawRect(draw_, &clearC, textCol_ * cellWidthPx_, textRow_ * cellHeightPx_, clearW, clearH);
			}
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


		/** Sets the window icon. 

		    The window icon must be an array of BGRA colors for the different icon sizes where the first element is the total size of the array followed by arbitrary icon sizes encoded by first 2 items representing the icon width and height followed by the actual pixels. 
		 */
		void setIcon(unsigned long * icon);

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

		/** Given current state as reported from X11, translates it to vterm::Key modifiers
		 */
		static unsigned GetStateModifiers(int state);

        /** Converts the KeySym and preexisting modifiers as reported by X11 into key. 

		    Because the modifiers are preexisting, but the terminal requires post-state, Shift, Ctrl, Alt and Win keys also update the modifiers based on whether they key was pressed, or released
         */
        static vterm::Key GetKey(KeySym k, unsigned modifiers, bool pressed);

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
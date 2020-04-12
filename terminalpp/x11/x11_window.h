#pragma once
#if (defined ARCH_UNIX && defined RENDERER_NATIVE)

#include "x11.h"

#include "x11_font.h"
#include "../window.h"

namespace tpp {

    using namespace ui2;

    class X11Window : public RendererWindow<X11Window, x11::Window> {
    public:

        //using Font = X11Font;

        ~X11Window() override;

        void repaint(Widget * widget) {
            MARK_AS_UNUSED(widget);
            // trigger a refresh
            XEvent e;
            memset(&e, 0, sizeof(XEvent));
            e.xexpose.type = Expose;
            e.xexpose.display = display_;
            e.xexpose.window = window_;
            X11Application::Instance()->xSendEvent(this, e, ExposureMask);
        }

        void setTitle(std::string const & value) override;

        void setIcon(Window::Icon icon) override;

        void setFullscreen(bool value = true) override;

        void show(bool value = true) override;

        void resize(int newWidth, int newHeight) override {
            if (newWidth != width())
                updateXftStructures(newWidth);
            RendererWindow::resize(newWidth, newHeight);
        }

    protected:

        void windowResized(int width, int height) override {
			if (buffer_ != 0) {
				XFreePixmap(display_, buffer_);
				buffer_ = 0;
			}
            RendererWindow::windowResized(width, height);
        }


        /** Destroys the renderer's window. 
         */
        void rendererClose() override {
            XDestroyWindow(display_, window_);
        }

        void requestClipboard(Widget * sender) override;

        void requestSelection(Widget * sender) override;

        void rendererSetClipboard(std::string const & contents) override;

        void rendererRegisterSelection(std::string const & contents, Widget * owner) override;

        void rendererClearSelection() override;

    private:
        friend class X11Application;
        friend class RendererWindow<X11Window, x11::Window>;

		class MotifHints {
		public:
			unsigned long   flags;
			unsigned long   functions;
			unsigned long   decorations;
			long            inputMode;
			unsigned long   status;
		};

        /** Creates the renderer window of appropriate size using the default font and zoom of 1.0. 
         */
        X11Window(std::string const & title, int cols, int rows);

        /** \name Rendering Functions
         */
        //@{
        void initializeDraw() {
            ASSERT(draw_ == nullptr);
            if (buffer_ == 0) {
                buffer_ = XCreatePixmap(display_, window_, widthPx_, heightPx_, DefaultDepth(display_, screen_));
                ASSERT(buffer_ != 0);
            }
            draw_ = XftDrawCreate(display_, buffer_, visual_, colorMap_);
        }

        void finalizeDraw() {
            changeBackgroundColor(backgroundColor());
            if (widthPx_ % cellWidth_ != 0)
                XftDrawRect(draw_, &bg_, width() * cellWidth_, 0, widthPx_ % cellWidth_, heightPx_);
            if (heightPx_ % cellHeight_ != 0)
                XftDrawRect(draw_, &bg_, 0, height() * cellHeight_, width(), heightPx_ % cellHeight_);
            // now bitblt the buffer
            XCopyArea(display_, buffer_, window_, gc_, 0, 0, widthPx_, heightPx_, 0, 0);
            XftDrawDestroy(draw_);
            draw_ = nullptr;
            XFlush(display_);
        }

        void initializeGlyphRun(int col, int row) {
            textSize_ = 0;
            textCol_ = col;
            textRow_ = row;
        }

        void addGlyph(int col, int row, Cell const & cell) {
            FT_UInt glyph = XftCharIndex(display_, font_->xftFont(), cell.codepoint());
            if (glyph == 0) {
                // draw glyph run so far and initialize a new glyph run
                drawGlyphRun();
                initializeGlyphRun(col, row);
                // obtain the fallback font and initialize the glyph run with it
                X11Font * oldFont = font_;
                font_ = font_->fallbackFor(cell.codepoint());
                text_[0].glyph = XftCharIndex(display_, font_->xftFont(), cell.codepoint());
                text_[0].x = textCol_ * cellWidth_ + font_->offsetLeft();
                text_[0].y = (textRow_ + 1 - font_->font().height()) * cellHeight_ + font_->ascent() + font_->offsetTop();
                ++textSize_;
                drawGlyphRun();
                initializeGlyphRun(col + font_->font().width(), row);
                font_ = oldFont;
            } else {
                if (textSize_ == 0) {
                    text_[0].x = textCol_ * cellWidth_ + font_->offsetLeft();
                    text_[0].y = (textRow_ + 1 - state_.font().height()) * cellHeight_ + font_->ascent() + font_->offsetTop();
                } else {
                    text_[textSize_].x = text_[textSize_ - 1].x + cellWidth_ * state_.font().width();
                    text_[textSize_].y = text_[textSize_ - 1].y;
                }
                text_[textSize_].glyph = glyph;
                ++textSize_;
            }
        }

        /** Updates the current font.
         */
        void changeFont(ui2::Font font) {
			font_ = X11Font::Get(font, cellHeight_, cellWidth_);
        }

        /** Updates the foreground color.
         */
        void changeForegroundColor(Color color) {
            fg_ = toXftColor(color);
        }

        /** Updates the background color. 
         */
        void changeBackgroundColor(Color color) {
            bg_ = toXftColor(color);
        }

        /** Updates the decoration color. 
         */
        void changeDecorationColor(Color color) {
            decor_ = toXftColor(color);
        }

        /** Draws the glyph run. 
         
            First clears the background with given background color, then draws the text and finally applies any decorations. 
         */
        void drawGlyphRun() {
            if (textSize_ == 0)
                return;
            int fontWidth = state_.font().width();
            int fontHeight = state_.font().height();
            // fill the background unless it is fully transparent
            if (bg_.color.alpha != 0)
			    XftDrawRect(draw_, &bg_, textCol_ * cellWidth_, (textRow_ + 1 - fontHeight) * cellHeight_, textSize_ * cellWidth_ * fontWidth, cellHeight_ * fontHeight);
            // draw the text
            if (!state_.font().blink() || BlinkVisible_) {
                XftDrawGlyphSpec(draw_, &fg_, font_->xftFont(), text_, textSize_);
                // deal with the attributes
                if (state_.font().underline())
					XftDrawRect(draw_, &decor_, textCol_ * cellWidth_, textRow_ * cellHeight_ + font_->underlineOffset(), cellWidth_ * textSize_, font_->underlineThickness());
                if (state_.font().strikethrough())
					XftDrawRect(draw_, &decor_, textCol_ * cellWidth_, textRow_ * cellHeight_ + font_->strikethroughOffset(), cellWidth_ * textSize_, font_->strikethroughThickness());
            }
            textCol_ += textSize_;
            textSize_ = 0;
        }

        void drawBorder(int col, int row, Border const & border, int widthThin, int widthThick) {
            int left = col * cellWidth_;
            int top = row * cellHeight_;
            int widthTop = border.top() == Border::Kind::None ? 0 : (border.top() == Border::Kind::Thick ? widthThick : widthThin);
            int widthLeft = border.left() == Border::Kind::None ? 0 : (border.left() == Border::Kind::Thick ? widthThick : widthThin);
            int widthBottom = border.bottom() == Border::Kind::None ? 0 : (border.bottom() == Border::Kind::Thick ? widthThick : widthThin);
            int widthRight = border.right() == Border::Kind::None ? 0 : (border.right() == Border::Kind::Thick ? widthThick : widthThin);

            if (widthTop != 0)
                XftDrawRect(draw_, &bg_, left, top, cellWidth_, widthTop);
            if (widthBottom != 0)
                XftDrawRect(draw_, &bg_, left, top + cellHeight_ - widthBottom, cellWidth_, widthBottom);
            if (widthLeft != 0) 
                XftDrawRect(draw_, &bg_, left, top + widthTop, widthLeft, cellHeight_ - widthTop - widthBottom);
            if (widthRight != 0) 
                XftDrawRect(draw_, &bg_, left + cellWidth_ - widthRight, top + widthTop, widthRight, cellHeight_ - widthTop - widthBottom);
        }
        //@}

        void updateXftStructures(int cols) {
            delete [] text_;
            text_ = new XftGlyphSpec[cols];
        }

		XftColor toXftColor(ui2::Color const& c) {
			XftColor result;
			result.color.red = c.r * 256;
			result.color.green = c.g * 256;
			result.color.blue = c.b * 256;
			result.color.alpha = c.a * 256;
			return result;
		}

        x11::Window window_;
        Display * display_;
        int screen_;
        Visual * visual_;
        Colormap colorMap_;
        XIC ic_;

        GC gc_;
        Pixmap buffer_;

		XftDraw * draw_;
		XftColor fg_;
		XftColor bg_;
        XftColor decor_;
        XftColor border_;
		X11Font * font_;

        XftGlyphSpec * text_;

        /** Text buffer rendering data.
         */
        unsigned textCol_;
        unsigned textRow_;
        unsigned textSize_;

		/** Info about the window state before fullscreen was triggered. 
		 */
        XWindowChanges fullscreenRestore_;


		/** Given current state as reported from X11, translates it to vterm::Key modifiers
		 */
		static unsigned GetStateModifiers(int state);

        /** Converts the KeySym and preexisting modifiers as reported by X11 into key. 

		    Because the modifiers are preexisting, but the terminal requires post-state, Shift, Ctrl, Alt and Win keys also update the modifiers based on whether they key was pressed, or released
         */
        static Key GetKey(KeySym k, unsigned modifiers, bool pressed);

        static void EventHandler(XEvent & e);


    }; // tpp::X11Window

} // namespace tpp

#endif
#pragma once
#if (defined ARCH_UNIX && defined RENDERER_NATIVE)

#include "x11.h"

#include "x11_font.h"
#include "../window.h"

namespace tpp {

    using namespace ui;

    class X11Window : public RendererWindow<X11Window, x11::Window> {
    public:
        /** Bring the font to the class' namespace so that the RendererWindow can find it. 
         */
        using Font = X11Font;

        ~X11Window() override;

        void setTitle(std::string const & value) override;

        void setIcon(Window::Icon icon) override;

        void setFullscreen(bool value = true) override;

        void show(bool value = true) override;

        void resize(Size const & newSize) override {
            if (newSize.width() != width())
                updateXftStructures(newSize.width());
            RendererWindow::resize(newSize);
        }

        void close() override {
            RendererWindow::close();
            XDestroyWindow(display_, window_);
        }

        void schedule(std::function<void()> event, Widget * widget) override;

    protected:

        void render(Rect const & rect) override {
            MARK_AS_UNUSED(rect);
            // trigger a refresh
            XEvent e;
            memset(&e, 0, sizeof(XEvent));
            e.xexpose.type = Expose;
            e.xexpose.display = display_;
            e.xexpose.window = window_;
            X11Application::Instance()->xSendEvent(this, e, ExposureMask);
        }

        void expose() {
            RendererWindow::render(Rect{size()});
        }

        void windowResized(int width, int height) override {
            XFreePixmap(display_, buffer_);
            buffer_ = XCreatePixmap(display_, window_, width, height, 32);
            RendererWindow::windowResized(width, height);
        }

        /** Handles FocusIn message sent to the window. 
         
            On certain x servers (such as vcxsrv) a newly created window get FocusOut message before having been focused first, which causes asserts in renderer to fail. The X11Window therefore keep a check whether the window has really been focused and only propagates the rendererFocusIn if the state is consistent. 
         */
        void focusIn() override {
            if (! rendererFocused())
                RendererWindow::focusIn();
        }

        /** Handles FocusOut message sent to the window. 
         
            On certain x servers (such as vcxsrv) a newly created window get FocusOut message before having been focused first, which causes asserts in renderer to fail. The X11Window therefore keep a check whether the window has really been focused and only propagates the rendererFocusIn if the state is consistent. 
         */
        void focusOut() override {
            if (rendererFocused())
                RendererWindow::focusOut();
        }

        void requestClipboard(Widget * sender) override;

        void requestSelection(Widget * sender) override;

        void setClipboard(std::string const & contents) override;

        void setSelection(std::string const & contents, Widget * owner) override;

        void clearSelection(Widget * sender) override;

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
        X11Window(std::string const & title, int cols, int rows, EventQueue & eventQueue);

        /** \name Rendering Functions
         */
        //@{
        void initializeDraw() {
            ASSERT(buffer_ != 0);
            ASSERT(draw_ == nullptr);
            draw_ = XftDrawCreate(display_, buffer_, visual_, colorMap_);
        }

        void finalizeDraw() {
            changeBackgroundColor(backgroundColor());
            if (sizePx_.width() % cellSize_.width() != 0)
                XftDrawRect(draw_, &bg_, width() * cellSize_.width(), 0, sizePx_.width() % cellSize_.width(), sizePx_.height());
            if (sizePx_.height() % cellSize_.height() != 0)
                XftDrawRect(draw_, &bg_, 0, height() * cellSize_.height(), sizePx_.width(), sizePx_.height() % cellSize_.height());
            // now bitblt the buffer
            XCopyArea(display_, buffer_, window_, gc_, 0, 0, sizePx_.width(), sizePx_.height(), 0, 0);
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
                text_[0].x = textCol_ * cellSize_.width() + font_->offset().x();
                text_[0].y = (textRow_ + 1 - font_->font().height()) * cellSize_.height() + font_->ascent() + font_->offset().y();
                ++textSize_;
                drawGlyphRun();
                initializeGlyphRun(col + font_->font().width(), row);
                font_ = oldFont;
            } else {
                if (textSize_ == 0) {
                    text_[0].x = textCol_ * cellSize_.width() + font_->offset().x();
                    text_[0].y = (textRow_ + 1 - state_.font().height()) * cellSize_.height() + font_->ascent() + font_->offset().y();
                } else {
                    text_[textSize_].x = text_[textSize_ - 1].x + cellSize_.width() * state_.font().width();
                    text_[textSize_].y = text_[textSize_ - 1].y;
                }
                text_[textSize_].glyph = glyph;
                ++textSize_;
            }
        }

        /** Updates the current font.
         */
        void changeFont(ui::Font font) {
			font_ = X11Font::Get(font, cellSize_);
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
			    XftDrawRect(draw_, &bg_, textCol_ * cellSize_.width(), (textRow_ + 1 - fontHeight) * cellSize_.height(), textSize_ * cellSize_.width() * fontWidth, cellSize_.height() * fontHeight);
            // draw the text
            if (!state_.font().blink() || BlinkVisible()) {
                XftDrawGlyphSpec(draw_, &fg_, font_->xftFont(), text_, textSize_);
                // deal with the attributes
                if (state_.font().underline()) {
                    if (state_.font().dashed()) {
                        for (size_t i = 0; i < textSize_; ++i) {
                            XftDrawRect(draw_, &decor_, (textCol_ + i) * cellSize_.width(), textRow_ * cellSize_.height() + font_->underlineOffset(), cellSize_.width() / 2, font_->underlineThickness());
                        }
                    } else {
					    XftDrawRect(draw_, &decor_, textCol_ * cellSize_.width(), textRow_ * cellSize_.height() + font_->underlineOffset(), cellSize_.width() * textSize_, font_->underlineThickness());
                    }
                }
                if (state_.font().strikethrough()) {
                    if (state_.font().dashed()) {
                        for (size_t i = 0; i < textSize_; ++i) {
                            XftDrawRect(draw_, &decor_, (textCol_ + i) * cellSize_.width(), textRow_ * cellSize_.height() + font_->strikethroughOffset(), cellSize_.width() / 2, font_->strikethroughThickness());
                        }
                    } else {
					    XftDrawRect(draw_, &decor_, textCol_ * cellSize_.width(), textRow_ * cellSize_.height() + font_->strikethroughOffset(), cellSize_.width() * textSize_, font_->strikethroughThickness());
                    }
                } 
            }
        }

        /** Draws the border. 
         
            Since the border is rendered over the contents and its color may be transparent, we can't use Xft's drawing, but have to revert to XRender which does the blending properly. 
         */
        void drawBorder(int col, int row, Border const & border, int widthThin, int widthThick) {
            int left = col * cellSize_.width();
            int top = row * cellSize_.height();
            int widthTop = border.top() == Border::Kind::None ? 0 : (border.top() == Border::Kind::Thick ? widthThick : widthThin);
            int widthLeft = border.left() == Border::Kind::None ? 0 : (border.left() == Border::Kind::Thick ? widthThick : widthThin);
            int widthBottom = border.bottom() == Border::Kind::None ? 0 : (border.bottom() == Border::Kind::Thick ? widthThick : widthThin);
            int widthRight = border.right() == Border::Kind::None ? 0 : (border.right() == Border::Kind::Thick ? widthThick : widthThin);

            if (widthTop != 0)
                XRenderFillRectangle(display_, PictOpOver, XftDrawPicture(draw_), &bg_.color, left, top, cellSize_.width(), widthTop);            
            if (widthBottom != 0)
                XRenderFillRectangle(display_, PictOpOver, XftDrawPicture(draw_), &bg_.color, left, top + cellSize_.height() - widthBottom, cellSize_.width(), widthBottom);
            if (widthLeft != 0) 
                XRenderFillRectangle(display_, PictOpOver, XftDrawPicture(draw_), &bg_.color, left, top + widthTop, widthLeft, cellSize_.height() - widthTop - widthBottom);
            if (widthRight != 0)
                XRenderFillRectangle(display_, PictOpOver, XftDrawPicture(draw_), &bg_.color, left + cellSize_.width() - widthRight, top + widthTop, widthRight, cellSize_.height() - widthTop - widthBottom); 
        }
        //@}

        void updateXftStructures(int cols) {
            delete [] text_;
            text_ = new XftGlyphSpec[cols];
        }

		XftColor toXftColor(ui::Color const& c) {
			XftColor result;
			result.color.red = (c.r << 8) + c.r;
			result.color.green = (c.g << 8) + c.g;
			result.color.blue = (c.b << 8) + c.b;
			result.color.alpha = (c.a << 8) + c.a;
            // premultiply
            result.color.red = result.color.red * result.color.alpha / 65535;
            result.color.green = result.color.green * result.color.alpha / 65535;
            result.color.blue = result.color.blue * result.color.alpha / 65535;
			return result;
		}

        x11::Window window_;
        Display * display_;
        int screen_;
        Visual * visual_ = nullptr;
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
		static Key GetStateModifiers(int state);

        /** Converts the KeySym and preexisting modifiers as reported by X11 into key. 

		    Because the modifiers are preexisting, but the terminal requires post-state, Shift, Ctrl, Alt and Win keys also update the modifiers based on whether they key was pressed, or released
         */
        static Key GetKey(KeySym k, Key modifiers, bool pressed);

        static void EventHandler(XEvent & e);


    }; // tpp::X11Window

} // namespace tpp

#endif
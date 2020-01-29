#pragma once
#if (defined ARCH_UNIX && defined RENDERER_NATIVE)

#include "x11.h"

#include "x11_font.h"
#include "../window.h"

namespace tpp {

    class X11Window : public RendererWindow<X11Window, x11::Window> {
    public:

        ~X11Window() override;
        
        void show() override;

        void hide() override {
            NOT_IMPLEMENTED;
        }

        /** Renderer closure request. 
         
            Terminates the window and the attached session.
         */
        void requestClose() override {
            XDestroyWindow(display_, window_);            
        }

        /** Schedules the window to be repainted.

            Instead of invalidating the rectange, WM_PAINT must explicitly be sent, as it may happen that different thread is already repainting the window, and therefore the request will be silenced (the window region is validated at the end of WM_PAINT). 
         */
		void requestRender(ui::Rect const & rect) override {
            MARK_AS_UNUSED(rect);
            // trigger a refresh
            XEvent e;
            memset(&e, 0, sizeof(XEvent));
            e.xexpose.type = Expose;
            e.xexpose.display = display_;
            e.xexpose.window = window_;
            X11Application::Instance()->xSendEvent(this, e, ExposureMask);
		}

        /** Sets the title of the window. 
         */
        void setTitle(std::string const & title) override {
            XSetStandardProperties(display_, window_, title.c_str(), nullptr, x11::None, nullptr, 0, nullptr);
        }

		/** Sets the window icon. 
		 */
        void setIcon(ui::RootWindow::Icon icon) override;

    protected:

        typedef RendererWindow<X11Window, x11::Window> Super;

        X11Window(std::string const & title, int cols, int rows, unsigned baseCellHeightPx);

        void updateSizePx(unsigned widthPx, unsigned heightPx) override {
			if (buffer_ != 0) {
				XFreePixmap(display_, buffer_);
				buffer_ = 0;
			}
            Window::updateSizePx(widthPx, heightPx);
			repaint();
        }

        void updateSize(int cols, int rows) override {
            updateXftStructures(cols);
            Window::updateSize(cols, rows);
			repaint();
        }

        void updateFullscreen(bool value) override;

        void updateZoom(double value) override;

        // renderer interface 

        // renderer clipboard interface 

        void requestClipboardContents() override;

        void requestSelectionContents() override;

        void setClipboard(std::string const & contents) override;

        void setSelection(std::string const & contents) override;

        void clearSelection() override;

        void yieldSelection();

    private:

        friend class RendererWindow<X11Window, x11::Window>;

        void updateXftStructures(int cols) {
            delete [] text_;
            text_ = new XftGlyphSpec[cols];
        }

        // rendering methods - we really want these to inline

        void initializeDraw() {
            ASSERT(draw_ == nullptr);
            if (buffer_ == 0) {
                buffer_ = XCreatePixmap(display_, window_, widthPx_, heightPx_, DefaultDepth(display_, screen_));
                ASSERT(buffer_ != 0);
            }
            draw_ = XftDrawCreate(display_, buffer_, visual_, colorMap_);
        }

        void finalizeDraw() {
            setBackgroundColor(rootWindow()->backgroundColor());
            if (widthPx_ % cellWidthPx_ != 0)
                XftDrawRect(draw_, &bg_, cols_ * cellWidthPx_, 0, widthPx_ % cellWidthPx_, heightPx_);
            if (heightPx_ % cellHeightPx_ != 0)
                XftDrawRect(draw_, &bg_, 0, rows_ * cellHeightPx_, widthPx_, heightPx_ % cellHeightPx_);
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

        void addGlyph(int col, int row, ui::Cell const & cell) {
            FT_UInt glyph = XftCharIndex(display_, font_->xftFont(), cell.codepoint());
            if (glyph == 0) {
                // draw glyph run so far and initialize a new glyph run
                drawGlyphRun();
                initializeGlyphRun(col, row);
                // obtain the fallback font and initialize the glyph run with it
                X11Font * oldFont = font_;
                font_ = font_->fallbackFor(cellWidthPx_, cellHeightPx_, cell.codepoint());
                text_[0].glyph = XftCharIndex(display_, font_->xftFont(), cell.codepoint());
                text_[0].x = textCol_ * cellWidthPx_ + font_->offsetLeft();
                text_[0].y = (textRow_ + 1 - font_->font().height()) * cellHeightPx_ + font_->ascent() + font_->offsetTop();
                ++textSize_;
                drawGlyphRun();
                initializeGlyphRun(col + font_->font().width(), row);
                font_ = oldFont;
            } else {
                if (textSize_ == 0) {
                    text_[0].x = textCol_ * cellWidthPx_ + font_->offsetLeft();
                    text_[0].y = (textRow_ + 1 - statusCell_.font().height()) * cellHeightPx_ + font_->ascent() + font_->offsetTop();
                } else {
                    text_[textSize_].x = text_[textSize_ - 1].x + cellWidthPx_ * statusCell_.font().width();
                    text_[textSize_].y = text_[textSize_ - 1].y;
                }
                text_[textSize_].glyph = glyph;
                ++textSize_;
            }
        }

        /** Updates the current font.
         */
        void setFont(ui::Font font) {
			font_ = X11Font::GetOrCreate(font, cellWidthPx_, cellHeightPx_);
        }

        /** Updates the foreground color.
         */
        void setForegroundColor(ui::Color color) {
            fg_ = toXftColor(color);
        }

        /** Updates the background color. 
         */
        void setBackgroundColor(ui::Color color) {
            bg_ = toXftColor(color);
        }

        /** Updates the decoration color. 
         */
        void setDecorationColor(ui::Color color) {
            decor_ = toXftColor(color);
        }
    
        /** Updates the decoration color. 
         */
        void setBorderColor(ui::Color color) {
            border_ = toXftColor(color);
        }

        /** Sets the attributes of the cell. 
         */
        void setAttributes(ui::Attributes const & attrs) {
            attrs_ = attrs;
        }

        /** Draws the glyph run. 
         
            First clears the background with given background color, then draws the text and finally applies any decorations. 
         */
        void drawGlyphRun() {
            if (textSize_ == 0)
                return;
            int fontWidth = statusCell_.font().width();
            int fontHeight = statusCell_.font().height();
            // fill the background unless it is fully transparent
            if (bg_.color.alpha != 0)
			    XftDrawRect(draw_, &bg_, textCol_ * cellWidthPx_, (textRow_ + 1 - fontHeight) * cellHeightPx_, textSize_ * cellWidthPx_ * fontWidth, cellHeightPx_ * fontHeight);
            // draw the text
            if (!attrs_.blink() || blinkVisible_)
                XftDrawGlyphSpec(draw_, &fg_, font_->xftFont(), text_, textSize_);
            // deal with the attributes
            if (!attrs_.emptyDecorations()) {
                if (attrs_.underline() && (!attrs_.blink() || blinkVisible_))
					XftDrawRect(draw_, &decor_, textCol_ * cellWidthPx_, textRow_ * cellHeightPx_ + font_->underlineOffset(), cellWidthPx_ * textSize_, font_->underlineThickness());
                if (attrs_.strikethrough() && (!attrs_.blink() || blinkVisible_))
					XftDrawRect(draw_, &decor_, textCol_ * cellWidthPx_, textRow_ * cellHeightPx_ + font_->strikethroughOffset(), cellWidthPx_ * textSize_, font_->strikethroughThickness());
            }
            textSize_ = 0;
        }

        void drawBorder(ui::Attributes attrs, int left, int top, int width) {
            int cLeft = left;
            int cTop = top;
            int cWidth = cellWidthPx_;
            int cHeight = width;
            // if top border is selected, draw the top line 
            if (attrs.borderTop()) {
                XftDrawRect(draw_, &border_, cLeft, cTop, cWidth, cHeight);
            // otherwise see if the left or right parts of the border should be drawn
            } else {
                if (attrs.borderLeft()) {
                    cWidth = width;
                    XftDrawRect(draw_, &border_, cLeft, cTop, cWidth, cHeight);
                }
                if (attrs.borderRight()) {
                    cWidth = width;
                    cLeft = left + cellWidthPx_ - width;
                    XftDrawRect(draw_, &border_, cLeft, cTop, cWidth, cHeight);
                }
            }
            // check the left and right border in the middle part
            cTop += width;
            cHeight = cellHeightPx_ - 2 * width;
            if (attrs.borderLeft()) {
                cLeft = left;
                cWidth = width;
                XftDrawRect(draw_, &border_, cLeft, cTop, cWidth, cHeight);
            }
            if (attrs.borderRight()) {
                cWidth = width;
                cLeft = left + cellWidthPx_ - width;
                XftDrawRect(draw_, &border_, cLeft, cTop, cWidth, cHeight);
            }
            // check if the bottom part should be drawn, first by checking whether the whole bottom part should be drawn, if not then by separate checking the left and right corner
            cTop = top + cellHeightPx_ - width;
            cHeight = width;
            if (attrs.borderBottom()) {
                cLeft = left;
                cWidth = cellWidthPx_;
                XftDrawRect(draw_, &border_, cLeft, cTop, cWidth, cHeight);
            } else {
                if (attrs.borderLeft()) {
                    cLeft = left;
                    cWidth = width;
                    XftDrawRect(draw_, &border_, cLeft, cTop, cWidth, cHeight);
                }
                if (attrs.borderRight()) {
                    cWidth = width;
                    cLeft = left + cellWidthPx_ - width;
                    XftDrawRect(draw_, &border_, cLeft, cTop, cWidth, cHeight);
                }
            }
        }

		XftColor toXftColor(ui::Color const& c) {
			XftColor result;
			result.color.red = c.r * 256;
			result.color.green = c.g * 256;
			result.color.blue = c.b * 256;
			result.color.alpha = c.a * 256;
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

		x11::Window window_;
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
        XftColor decor_;
        XftColor border_;
		X11Font * font_;

        XftGlyphSpec * text_;

        /** Text buffer rendering data.
         */
        unsigned textCol_;
        unsigned textRow_;
        unsigned textSize_;
        ui::Attributes attrs_;

        std::mutex drawGuard_;
        std::atomic<bool> invalidate_;

		/** Info about the window state before fullscreen was triggered. 
		 */
        XWindowChanges fullscreenRestore_;

		/** Given current state as reported from X11, translates it to vterm::Key modifiers
		 */
		static unsigned GetStateModifiers(int state);

        /** Converts the KeySym and preexisting modifiers as reported by X11 into key. 

		    Because the modifiers are preexisting, but the terminal requires post-state, Shift, Ctrl, Alt and Win keys also update the modifiers based on whether they key was pressed, or released
         */
        static ui::Key GetKey(KeySym k, unsigned modifiers, bool pressed);

        static void EventHandler(XEvent & e);
    }; // X11Window

} // namespace tpp

#endif
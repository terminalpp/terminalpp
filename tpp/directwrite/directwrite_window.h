#pragma once
#if (defined ARCH_WINDOWS)

#include <unordered_map>

#include "helpers/time.h"

#include "../window.h"

#include "directwrite_font.h"

namespace tpp {

    class DirectWriteWindow : public RendererWindow<DirectWriteWindow, HWND> {
    public:

        ~DirectWriteWindow() override;

        void show() override {
            ShowWindow(hWnd_, SW_SHOWNORMAL);
        }

        void hide() override {
            NOT_IMPLEMENTED;
        }

        /** Renderer closure request. 
         
            Terminates the window and the attached session.
         */
        void requestClose() override {
            PostMessage(hWnd_, WM_CLOSE, 0, 0);
        }

        /** Schedules the window to be repainted.

            Instead of invalidating the rectange, WM_PAINT must explicitly be sent, as it may happen that different thread is already repainting the window, and therefore the request will be silenced (the window region is validated at the end of WM_PAINT). 
         */
		void requestRender(ui::Rect const & rect) override {
            MARK_AS_UNUSED(rect);
            PostMessage(hWnd_, WM_PAINT, 0, 0);
		}

        /** Sets the title of the window. 
         */
        void setTitle(std::string const & title) override {
            MARK_AS_UNUSED(title);
            PostMessage(hWnd_, WM_USER, DirectWriteApplication::MSG_TITLE_CHANGE, 0);                
        }

        void setIcon(ui::RootWindow::Icon icon) override {
            DirectWriteApplication *app = DirectWriteApplication::Instance();
            WPARAM iconHandle;
            switch (icon) {
                case ui::RootWindow::Icon::Notification:
                    iconHandle = reinterpret_cast<WPARAM>(app->iconNotification_);
                    break;
                default:
                    iconHandle = reinterpret_cast<WPARAM>(app->iconDefault_);
                    break;
            }
            PostMessage(hWnd_, WM_SETICON, ICON_BIG, iconHandle);
            PostMessage(hWnd_, WM_SETICON, ICON_SMALL, iconHandle);
        }

    protected:

        DirectWriteWindow(std::string const & title, int cols, int rows, unsigned baseCellHeightPx);

        void updateSizePx(unsigned widthPx, unsigned heightPx) override {
            //widthPx -= frameWidthPx_;
            //heightPx -= frameHeightPx_;
			if (rt_ != nullptr) {
				D2D1_SIZE_U size = D2D1::SizeU(widthPx, heightPx);
				rt_->Resize(size);
			}
            Window::updateSizePx(widthPx, heightPx);
			repaint();
        }

        void updateSize(int cols, int rows) override {
			if (rt_ != nullptr) 
                updateDirectWriteStructures(cols);
            Window::updateSize(cols, rows);
			repaint();
        }


        void updateFullscreen(bool value) override;

        void updateZoom(double value) override;

        /** Registers mouse button down.
          
            Starts mouse capture if no mouse button has been pressed previously, which allows the terminal window to track mouse movement outside of the window if at least one mouse button is pressed. 
         */ 
        virtual void mouseDown(int x, int y, ui::MouseButton button) override {
            if (++mouseButtonsDown_ == 1)
                SetCapture(hWnd_);
            Window::mouseDown(x, y, button);
        }

        /** Registers mouse button up. 
          
            If there are no more pressed buttons left, releases the mouse capture previously obtained. 
         */
        virtual void mouseUp(int x, int y, ui::MouseButton button) override {
            // just a bit of defensive programming to make sure if weird capture events wreak havoc the terminal is safe(ish)
            if (mouseButtonsDown_ > 0 && --mouseButtonsDown_ == 0)
                ReleaseCapture();
            Window::mouseUp(x, y, button);
        }

        // renderer clipboard interface 

        void requestClipboardPaste() override;

        void requestSelectionPaste() override;

        void setClipboard(std::string const & contents) override;

        void setSelection(std::string const & contents) override;

        void clearSelection() override;

    private:
        friend class DirectWriteApplication;
        friend class RendererWindow<DirectWriteWindow, HWND>;

        /** Updates the glyph run structures so that up to an entire line can be fit in a single glyph run. 
         */
        void updateDirectWriteStructures(int cols);

        // rendering methods - we really want these to inline

        void initializeDraw() {
            rt_->BeginDraw();
        }

        void finalizeDraw() {
            setBackgroundColor(rootWindow()->background());
            if (widthPx_ % cellWidthPx_ != 0) {
                D2D1_RECT_F rect = D2D1::RectF(
                    static_cast<FLOAT>(cols_ * cellWidthPx_),
                    static_cast<FLOAT>(0),
                    static_cast<FLOAT>(widthPx_),
                    static_cast<FLOAT>(heightPx_)
                );
    			rt_->FillRectangle(rect, bg_.Get());
            }
            if (heightPx_ % cellHeightPx_ != 0) {
                D2D1_RECT_F rect = D2D1::RectF(
                    static_cast<FLOAT>(0),
                    static_cast<FLOAT>(rows_ * cellHeightPx_),
                    static_cast<FLOAT>(widthPx_),
                    static_cast<FLOAT>(heightPx_)
                );
    			rt_->FillRectangle(rect, bg_.Get());
            }
            rt_->EndDraw();
        }

        void initializeGlyphRun(int col, int row) {
            glyphRun_.glyphCount = 0;
            textSizeCells_ = 0;
            glyphRunCol_ = col;
            glyphRunRow_ = row;
        }

        void addGlyph(int col, int row, ui::Cell const & cell) {
            UINT32 cp = cell.codepoint();
            font_->fontFace()->GetGlyphIndices(&cp, 1, glyphIndices_ + glyphRun_.glyphCount);
            // if the glyph is not in the font, try callback
            if (glyphIndices_[glyphRun_.glyphCount] == 0) {
                // draw glyph run so far and initialize a new glyph run
                drawGlyphRun();
                initializeGlyphRun(col, row);
                // obtain the fallback font and point the glyph run towards it
                DirectWriteFont * oldFont = font_;
                font_ = font_->fallbackFor(cellWidthPx_, cellHeightPx_, cp);
                glyphRun_.fontFace = font_->fontFace();
                glyphRun_.fontEmSize = font_->sizeEm();
                const_cast<float *>(glyphRun_.glyphAdvances)[glyphRun_.glyphCount] = static_cast<float>(cellWidthPx_ * font_->font().width());
                font_->fontFace()->GetGlyphIndices(&cp, 1, glyphIndices_);
                glyphRun_.glyphCount = 1;
                textSizeCells_ += helpers::Char::ColumnWidth(cell.codepoint());
                drawGlyphRun();
                // revert the font back to what we had before and reinitialize the glyphrun to start at next character
                font_ = oldFont;
                initializeGlyphRun(col + font_->font().width(), row);
                glyphRun_.fontFace = font_->fontFace();
                glyphRun_.fontEmSize = font_->sizeEm();
            } else {
                const_cast<float *>(glyphRun_.glyphAdvances)[glyphRun_.glyphCount] = static_cast<float>(cellWidthPx_ * font_->font().width());
                ++glyphRun_.glyphCount;
                textSizeCells_ += helpers::Char::ColumnWidth(cell.codepoint());
            }
        }

        /** Updates the current font.
         */
        void setFont(ui::Font font) {
			font_ = DirectWriteFont::GetOrCreate(font, cellWidthPx_, cellHeightPx_);
			glyphRun_.fontFace = font_->fontFace();
			glyphRun_.fontEmSize = font_->sizeEm();
        }

        /** Updates the foreground color.
         */
        void setForegroundColor(ui::Color color) {
			fg_->SetColor(D2D1::ColorF(color.toRGB(), color.floatAlpha()));
        }

        /** Updates the background color. 
         */
        void setBackgroundColor(ui::Color color) {
		    bg_->SetColor(D2D1::ColorF(color.toRGB(), color.floatAlpha()));
        }

        /** Updates the decoration color. 
         */
        void setDecorationColor(ui::Color color) {
            decor_->SetColor(D2D1::ColorF(color.toRGB(), color.floatAlpha()));
        }

        /** Updates the border color. 
         */
        void setBorderColor(ui::Color color) {
            border_->SetColor(D2D1::ColorF(color.toRGB(), color.floatAlpha()));
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
            if (glyphRun_.glyphCount == 0)
                return;
            // get the glyph run rectange
			D2D1_RECT_F rect = D2D1::RectF(
				static_cast<FLOAT>(glyphRunCol_ * cellWidthPx_),
				static_cast<FLOAT>((glyphRunRow_ + 1 - statusCell_.font().height()) * cellHeightPx_),
				static_cast<FLOAT>((glyphRunCol_ + textSizeCells_ * statusCell_.font().width()) * cellWidthPx_),
				static_cast<FLOAT>((glyphRunRow_ + 1) * cellHeightPx_)
			);
            // fill it with the background
			rt_->FillRectangle(rect, bg_.Get());
            // if there is border to be drawn below the text, draw it now
            if (statusCell_.attributes().border() && ! statusCell_.attributes().borderAbove())
                drawBorders(rect);
#ifdef SHOW_LINE_ENDINGS
            if (attrs_.endOfLine()) {
                auto oldC = bg_->GetColor();
                bg_->SetColor(D2D1::ColorF(0xffff00, 1.0f));
                rt_->DrawRectangle(rect, bg_.Get());
                bg_->SetColor(oldC);
            }
#endif
            // determine the originl and draw the glyph run
            D2D1_POINT_2F origin = D2D1::Point2F(
                static_cast<float>(glyphRunCol_* cellWidthPx_ + font_->offsetLeft()),
                ((glyphRunRow_ + 1 - statusCell_.font().height()) * cellHeightPx_ + font_->ascent()) + font_->offsetTop());
            if (!attrs_.blink() || blinkVisible_)
                rt_->DrawGlyphRun(origin, &glyphRun_, fg_.Get());
            // see if there are any attributes to be drawn 
            if (!attrs_.emptyDecorations()) {
                if (attrs_.underline() && (!attrs_.blink() || blinkVisible_)) {
					D2D1_POINT_2F start = origin;
					start.y -= font_->underlineOffset();
					D2D1_POINT_2F end = start;
					end.x += textSizeCells_ * cellWidthPx_;
					rt_->DrawLine(start, end, decor_.Get(), font_->underlineThickness());
                }
                if (attrs_.strikethrough() && (!attrs_.blink() || blinkVisible_)) {
					D2D1_POINT_2F start = origin;
					start.y -= font_->strikethroughOffset();
					D2D1_POINT_2F end = start;
					end.x += textSizeCells_ * cellWidthPx_;
					rt_->DrawLine(start, end, decor_.Get(), font_->strikethroughThickness());
                }
            }
            // if there is border to be drawn above the text, draw it now
            if (statusCell_.attributes().border() && statusCell_.attributes().borderAbove())
                drawBorders(rect);
			glyphRun_.glyphCount = 0;
            textSizeCells_ = 0;
        }

        /** Draws the cell borders (if any)
         */
        void drawBorders(D2D1_RECT_F const & rect) {
            //ASSERT(statusCell_.attributes().border()); 
            ui::Attributes attrs = statusCell_.attributes();
            unsigned cw = cellWidthPx_ * statusCell_.font().width();
            unsigned ht = attrs.borderThick() ? (cellHeightPx_ / 2) : (cellHeightPx_ / 4);
            unsigned vt = attrs.borderThick() ? (cellWidthPx_ / 2) : (cellWidthPx_ / 4);
            // if there text size in cells is twice the number of glyphs (double width glyphs), multiply cell width by two, otherwise assert that the text cells size is equal to number of glyphs (mixed single/double width characters not allowed in bordered cells)
            if (textSizeCells_ == glyphRun_.glyphCount * 2)
                cw *= 2;
            else
                ASSERT(textSizeCells_ == glyphRun_.glyphCount);
            // draw borders for each cell in question
            for (unsigned i = 0; i < glyphRun_.glyphCount; ++i) {
                // create the cellRectangle, prepared for top border
                D2D1_RECT_F cellRect = D2D1::RectF(
                    rect.left + i * cw, 
                    rect.top,
                    rect.left + (i + 1) * cw,
                    rect.top + ht
                );
                // if top border is selected, draw the top line 
                if (attrs.borderTop()) {
                    rt_->FillRectangle(cellRect, border_.Get());
                // otherwise see if the left or right parts of the border should be drawn
                } else {
                    if (attrs.borderLeft()) {
                        cellRect.right = cellRect.left + vt;
                        rt_->FillRectangle(cellRect, border_.Get());
                    }
                    if (attrs.borderRight()) {
                        cellRect.right = rect.left + (i + 1) * cw;
                        cellRect.left = cellRect.right - vt;
                        rt_->FillRectangle(cellRect, border_.Get());
                    }
                }
                // check the left and right border in the middle part
                cellRect.top = cellRect.bottom;
                cellRect.bottom = rect.bottom - ht;
                if (attrs.borderLeft()) {
                    cellRect.left = rect.left + i * cw;
                    cellRect.right = cellRect.left + vt;
                    rt_->FillRectangle(cellRect, border_.Get());
                }
                if (attrs.borderRight()) {
                    cellRect.right = rect.left + (i + 1) * cw;
                    cellRect.left = cellRect.right - vt;
                    rt_->FillRectangle(cellRect, border_.Get());
                }
                // check if the bottom part should be drawn, first by checking whether the whole bottom part should be drawn, if not then by separate checking the left and right corner
                cellRect.top = cellRect.bottom;
                cellRect.bottom = rect.bottom;
                if (attrs.borderBottom()) {
                    cellRect.left = rect.left + i * cw;
                    cellRect.right = rect.left + (i + 1) * cw;
                    rt_->FillRectangle(cellRect, border_.Get());
                } else {
                    if (attrs.borderLeft()) {
                        cellRect.left = rect.left + i * cw;
                        cellRect.right = cellRect.left + vt;
                        rt_->FillRectangle(cellRect, border_.Get());
                    }
                    if (attrs.borderRight()) {
                        cellRect.right = rect.left + (i + 1) * cw;
                        cellRect.left = cellRect.right - vt;
                        rt_->FillRectangle(cellRect, border_.Get());
                    }
                }
            }
        }

        /* Window handle. */
        HWND hWnd_;

        /* Window placement to which the window should be returned when fullscreen mode is toggled off. */
        WINDOWPLACEMENT wndPlacement_;

        /* Dimensions of the window frame so that we can the window client area can be calculated. */
        unsigned frameWidthPx_;
        unsigned frameHeightPx_;

		/* Direct X structures. */
		Microsoft::WRL::ComPtr<ID2D1HwndRenderTarget> rt_;
		Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> fg_; // foreground (text) style
		Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> bg_; // background style
		Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> decor_; // decoration color
        Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> border_; // border color
		DirectWriteFont* font_;
        ui::Attributes attrs_;

		DWRITE_GLYPH_RUN glyphRun_;
        unsigned textSizeCells_;
		UINT16 * glyphIndices_;
		FLOAT * glyphAdvances_;
		DWRITE_GLYPH_OFFSET * glyphOffsets_;
		int glyphRunCol_;
		int glyphRunRow_;

        /* Number of mouse buttons currently pressed so that we know when to set & release mouse capture.
         */
        unsigned mouseButtonsDown_;

        static ui::Key GetKey(unsigned vk);

        static LRESULT CALLBACK EventHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);


    };


} // namespace tpp



#endif
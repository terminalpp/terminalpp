#pragma once
#if (defined ARCH_WINDOWS && defined RENDERER_NATIVE)

#include <unordered_map>

#include "helpers/time.h"

#include "../window.h"

#include "directwrite_font.h"

namespace tpp2 {

    using namespace ui2;

    class DirectWriteWindow : public RendererWindow<DirectWriteWindow, HWND> {
    public:

        /** Bring the font to the class' namespace so that the RendererWindow can find it. 
         */
        using Font = DirectWriteFont;

        ~DirectWriteWindow() override {
            UnregisterWindowHandle(hWnd_);
        }

        /** Repainting the window simply sends the repaint message to it. 
         
            Since each repaint redraws the entire window, the widget is ignored. 
         */
        void repaint(Widget * widget) override {
            MARK_AS_UNUSED(widget);
            PostMessage(hWnd_, WM_PAINT, 0, 0);
        }

        void setTitle(std::string const & value) override;

        void setFullscreen(bool value = true) override;

        void show(bool value = true) override {
            ShowWindow(hWnd_, SW_SHOWNORMAL);
        }



        void resize(int newWidth, int newHeight) override {
            if (newWidth != width())
                updateDirectWriteStructures(newWidth);
            RendererWindow::resize(newWidth, newHeight);
        }

    protected:

        /** Destroys the renderer's window. 
         */
        void rendererClose() override {
            DestroyWindow(hWnd_);
        }

    private:

        friend class DirectWriteApplication;

        friend class RendererWindow<DirectWriteWindow, HWND>;

        /** Creates the renderer window of appropriate size using the default font and zoom of 1.0. 
         
            TODO in the future, I want the zoom to be configurable. 
         */
        DirectWriteWindow(std::string const & title, int width, int height);

        /** \name Rendering Functions
         */
        //@{

        void initializeDraw() {
            rt_->BeginDraw();
        }

        void finalizeDraw() {
            changeBackgroundColor(backgroundColor());
            if (widthPx_ % cellWidth_ != 0) {
                D2D1_RECT_F rect = D2D1::RectF(
                    static_cast<FLOAT>(width() * cellWidth_),
                    static_cast<FLOAT>(0),
                    static_cast<FLOAT>(widthPx_),
                    static_cast<FLOAT>(heightPx_)
                );
    			rt_->FillRectangle(rect, bg_.Get());
            }
            if (heightPx_ % cellHeight_ != 0) {
                D2D1_RECT_F rect = D2D1::RectF(
                    static_cast<FLOAT>(0),
                    static_cast<FLOAT>(height() * cellHeight_),
                    static_cast<FLOAT>(widthPx_),
                    static_cast<FLOAT>(heightPx_)
                );
    			rt_->FillRectangle(rect, bg_.Get());
            }
            rt_->EndDraw();
        }

        void initializeGlyphRun(int col, int row) {
            glyphRun_.glyphCount = 0;
            glyphRunCol_ = col;
            glyphRunRow_ = row;
        }

        void addGlyph(int col, int row, Cell const & cell) {
            UINT32 cp = cell.codepoint();
            font_->fontFace()->GetGlyphIndices(&cp, 1, glyphIndices_ + glyphRun_.glyphCount);
            // if the glyph is not in the font, try callback
            if (glyphIndices_[glyphRun_.glyphCount] == 0) {
                // draw glyph run so far and initialize a new glyph run
                drawGlyphRun();
                initializeGlyphRun(col, row);
                // obtain the fallback font and point the glyph run towards it
                DirectWriteFont * oldFont = font_;
                font_ = font_->fallbackFor(cp);
                glyphRun_.fontFace = font_->fontFace();
                glyphRun_.fontEmSize = font_->sizeEm();
                const_cast<float *>(glyphRun_.glyphAdvances)[glyphRun_.glyphCount] = static_cast<float>(cellWidth_ * font_->font().width());
                font_->fontFace()->GetGlyphIndices(&cp, 1, glyphIndices_);
                glyphRun_.glyphCount = 1;
                drawGlyphRun();
                // revert the font back to what we had before and reinitialize the glyphrun to start at next character
                font_ = oldFont;
                initializeGlyphRun(col + font_->font().width(), row);
                glyphRun_.fontFace = font_->fontFace();
                glyphRun_.fontEmSize = font_->sizeEm();
            } else {
                const_cast<float *>(glyphRun_.glyphAdvances)[glyphRun_.glyphCount] = static_cast<float>(cellWidth_ * font_->font().width());
                ++glyphRun_.glyphCount;
            }
        }

        /** Updates the current font.
         */
        void changeFont(ui2::Font font) {
			font_ = DirectWriteFont::Get(font, cellHeight_, cellWidth_);
			glyphRun_.fontFace = font_->fontFace();
			glyphRun_.fontEmSize = font_->sizeEm();
        }

        /** Updates the foreground color.
         */
        void changeForegroundColor(Color color) {
			fg_->SetColor(D2D1::ColorF(color.toRGB(), color.floatAlpha()));
        }

        /** Updates the background color. 
         */
        void changeBackgroundColor(Color color) {
		    bg_->SetColor(D2D1::ColorF(color.toRGB(), color.floatAlpha()));
        }

        /** Updates the decoration color. 
         */
        void changeDecorationColor(Color color) {
            decor_->SetColor(D2D1::ColorF(color.toRGB(), color.floatAlpha()));
        }

        /** Updates the border color. 
         */
        void changeBorderColor(Color color) {
            border_->SetColor(D2D1::ColorF(color.toRGB(), color.floatAlpha()));
        }

        /** Draws the glyph run. 
         
            First clears the background with given background color, then draws the text and finally applies any decorations. 
         */
        void drawGlyphRun() {
            if (glyphRun_.glyphCount == 0)
                return;
            // get the glyph run rectange
			D2D1_RECT_F rect = D2D1::RectF(
				static_cast<FLOAT>(glyphRunCol_ * cellWidth_),
				static_cast<FLOAT>((glyphRunRow_ + 1 - state_.font().height()) * cellHeight_),
				static_cast<FLOAT>((glyphRunCol_ + glyphRun_.glyphCount * state_.font().width()) * cellWidth_),
				static_cast<FLOAT>((glyphRunRow_ + 1) * cellHeight_)
			);
            // fill it with the background
			rt_->FillRectangle(rect, bg_.Get());
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
                static_cast<float>(glyphRunCol_* cellWidth_ + font_->offsetLeft()),
                ((glyphRunRow_ + 1 - state_.font().height()) * cellHeight_ + font_->ascent()) + font_->offsetTop());
            //if (!attrs_.blink() || blinkVisible_)
            {
                rt_->DrawGlyphRun(origin, &glyphRun_, fg_.Get());
                // see if there are any attributes to be drawn 
                if (state_.font().underline()) {
                    D2D1_POINT_2F start = origin;
                    start.y -= font_->underlineOffset();
                    D2D1_POINT_2F end = start;
                    end.x += glyphRun_.glyphCount * cellWidth_;
                    rt_->DrawLine(start, end, decor_.Get(), font_->underlineThickness());
                }
                if (state_.font().strikethrough()) {
                    D2D1_POINT_2F start = origin;
                    start.y -= font_->strikethroughOffset();
                    D2D1_POINT_2F end = start;
                    end.x += glyphRun_.glyphCount * cellWidth_;
                    rt_->DrawLine(start, end, decor_.Get(), font_->strikethroughThickness());
                }
            }
			glyphRun_.glyphCount = 0;
        }

        //@}

        /** Updates the glyph run structures so that up to an entire line can be fit in a single glyph run. 
         */
        void updateDirectWriteStructures(int cols);

        /* Window handle. */
        HWND hWnd_;

        /* Window placement to which the window should be returned when fullscreen mode is toggled off. */
        WINDOWPLACEMENT wndPlacement_;

        /* Dimensions of the window frame so that we can the window client area can be calculated. */
        unsigned frameWidth_;
        unsigned frameHeight_;

		/* Direct X structures. */
		Microsoft::WRL::ComPtr<ID2D1HwndRenderTarget> rt_;
		Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> fg_; // foreground (text) style
		Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> bg_; // background style
		Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> decor_; // decoration color
        Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> border_; // border color
		DirectWriteFont* font_;

		DWRITE_GLYPH_RUN glyphRun_;
		UINT16 * glyphIndices_;
		FLOAT * glyphAdvances_;
		DWRITE_GLYPH_OFFSET * glyphOffsets_;
		int glyphRunCol_;
		int glyphRunRow_;


	    static Key GetKey(unsigned vk);

        static LRESULT CALLBACK EventHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

    }; // tpp::DirectWriteWindow


} // namespace tpp

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

        typedef RendererWindow<DirectWriteWindow, HWND> Super;

        DirectWriteWindow(std::string const & title, int cols, int rows, unsigned baseCellHeightPx);

        void updateSizePx(unsigned widthPx, unsigned heightPx) override {
            //widthPx -= frameWidthPx_;
            //heightPx -= frameHeightPx_;
			if (rt_ != nullptr) {
				D2D1_SIZE_U size = D2D1::SizeU(widthPx, heightPx);
				rt_->Resize(size);
			}
            Super::updateSizePx(widthPx, heightPx);
			repaint();
        }

        void updateSize(int cols, int rows) override {
			if (rt_ != nullptr) 
                updateDirectWriteStructures(cols);
            Super::updateSize(cols, rows);
			repaint();
        }


        void updateFullscreen(bool value) override;

        void updateZoom(double value) override;

        /** Registers mouse button down.
          
            Starts mouse capture if no mouse button has been pressed previously, which allows the terminal window to track mouse movement outside of the window if at least one mouse button is pressed. 
         */ 
        void mouseDown(int x, int y, ui::MouseButton button) override {
            Super::mouseDown(x, y, button);
            if (mouseButtonsDown_ == 1)
                SetCapture(hWnd_);
        }

        /** Registers mouse button up. 
          
            If there are no more pressed buttons left, releases the mouse capture previously obtained. 
         */
        void mouseUp(int x, int y, ui::MouseButton button) override {
            Super::mouseUp(x, y, button);
            if (mouseButtonsDown_ == 0)
                ReleaseCapture();
        }

        /** Mouse moves. 
         
            Triggers the root window's mouse move event, but also triggers the mouseEnter if this is the first mouse move on the window and register for mouse leave event in such case. 

            This is because in Win32 there is no mouse enter event (first mouse move is effectively mouse enter) and mouse leave is only returned if the application explicitly asks for its tracking (which is valid only for one mouse leave trigger and therefore must be renewed everytime mouse enters the window).
         */
        void mouseMove(int x, int y) override {
            // enable tracking if not enabled, also do mouseEnter on the root window? 
            if (! mouseLeaveTracked_) {
                TRACKMOUSEEVENT tm;
                tm.cbSize = sizeof(tm);
                tm.dwFlags = TME_LEAVE;
                tm.hwndTrack = hWnd_;
                mouseLeaveTracked_ = TrackMouseEvent(&tm);
                ASSERT(mouseLeaveTracked_);
            }
            Super::mouseMove(x, y);
        }

        // renderer clipboard interface 

        void requestClipboardContents() override;

        void requestSelectionContents() override;

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
            setBackgroundColor(rootWindow()->backgroundColor());
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
                drawGlyphRun();
                // revert the font back to what we had before and reinitialize the glyphrun to start at next character
                font_ = oldFont;
                initializeGlyphRun(col + font_->font().width(), row);
                glyphRun_.fontFace = font_->fontFace();
                glyphRun_.fontEmSize = font_->sizeEm();
            } else {
                const_cast<float *>(glyphRun_.glyphAdvances)[glyphRun_.glyphCount] = static_cast<float>(cellWidthPx_ * font_->font().width());
                ++glyphRun_.glyphCount;
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
				static_cast<FLOAT>((glyphRunCol_ + glyphRun_.glyphCount * statusCell_.font().width()) * cellWidthPx_),
				static_cast<FLOAT>((glyphRunRow_ + 1) * cellHeightPx_)
			);
            // fill it with the background
			rt_->FillRectangle(rect, bg_.Get());
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
					end.x += glyphRun_.glyphCount * cellWidthPx_;
					rt_->DrawLine(start, end, decor_.Get(), font_->underlineThickness());
                }
                if (attrs_.strikethrough() && (!attrs_.blink() || blinkVisible_)) {
					D2D1_POINT_2F start = origin;
					start.y -= font_->strikethroughOffset();
					D2D1_POINT_2F end = start;
					end.x += glyphRun_.glyphCount * cellWidthPx_;
					rt_->DrawLine(start, end, decor_.Get(), font_->strikethroughThickness());
                }
            }
			glyphRun_.glyphCount = 0;
        }

        void drawBorder(ui::Attributes attrs, int left, int top, int width) {
            float fLeft = static_cast<float>(left);
            float fTop = static_cast<float>(top);
            float fWidth = static_cast<float>(width);
            D2D1_RECT_F rect = D2D1::RectF(fLeft, fTop, fLeft + cellWidthPx_, fTop + fWidth);
            // if top border is selected, draw the top line 
            if (attrs.borderTop()) {
                rt_->FillRectangle(rect, border_.Get());
            // otherwise see if the left or right parts of the border should be drawn
            } else {
                if (attrs.borderLeft()) {
                    rect.right = fLeft + fWidth;
                    rt_->FillRectangle(rect, border_.Get());
                }
                if (attrs.borderRight()) {
                    rect.right = fLeft + cellWidthPx_;
                    rect.left = rect.right - fWidth;
                    rt_->FillRectangle(rect, border_.Get());
                }
            }
            // check the left and right border in the middle part
            rect.top = rect.bottom;
            rect.bottom = fTop + cellHeightPx_ - fWidth;
            if (attrs.borderLeft()) {
                rect.left = fLeft;
                rect.right = fLeft + fWidth;
                rt_->FillRectangle(rect, border_.Get());
            }
            if (attrs.borderRight()) {
                rect.right = fLeft + cellWidthPx_;
                rect.left = rect.right - fWidth;
                rt_->FillRectangle(rect, border_.Get());
            }
            // check if the bottom part should be drawn, first by checking whether the whole bottom part should be drawn, if not then by separate checking the left and right corner
            rect.top = rect.bottom;
            rect.bottom = fTop + cellHeightPx_;
            if (attrs.borderBottom()) {
                rect.left = fLeft;
                rect.right = fLeft + cellWidthPx_;;
                rt_->FillRectangle(rect, border_.Get());
            } else {
                if (attrs.borderLeft()) {
                    rect.left = fLeft;
                    rect.right = fLeft + fWidth;
                    rt_->FillRectangle(rect, border_.Get());
                }
                if (attrs.borderRight()) {
                    rect.right = fLeft + cellWidthPx_;
                    rect.left = rect.right - fWidth;
                    rt_->FillRectangle(rect, border_.Get());
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
		UINT16 * glyphIndices_;
		FLOAT * glyphAdvances_;
		DWRITE_GLYPH_OFFSET * glyphOffsets_;
		int glyphRunCol_;
		int glyphRunRow_;

        /** Determines whether the mouse leave action is currentlty being tracked. 
         */
        bool mouseLeaveTracked_;

        static ui::Key GetKey(unsigned vk);

        static LRESULT CALLBACK EventHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);


    };


} // namespace tpp



#endif
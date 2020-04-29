#pragma once
#if (defined ARCH_WINDOWS && defined RENDERER_NATIVE)

#include <unordered_map>

#include "helpers/time.h"

#include "../window.h"

#include "directwrite_font.h"

namespace tpp {

    using namespace ui;

    class DirectWriteWindow : public RendererWindow<DirectWriteWindow, HWND> {
    public:

        /** Bring the font to the class' namespace so that the RendererWindow can find it. 
         */
        using Font = DirectWriteFont;

        ~DirectWriteWindow() override {
            UnregisterWindowHandle(hWnd_);
        }

        /** Repaint request for given widget. 
         
            The WM_PAINT message is sent with the appropriate widget. 
         */
        void repaint(Widget * widget) override {
            PostMessage(hWnd_, WM_PAINT, 0, reinterpret_cast<LPARAM>(widget));
        }

        void setTitle(std::string const & value) override;

        void setIcon(Window::Icon icon) override;

        void setFullscreen(bool value = true) override;

        void show(bool value = true) override {
            if (value)
                ShowWindow(hWnd_, SW_SHOWNORMAL);
            else
                ShowWindow(hWnd_, SW_HIDE);
        }

        void resize(int newWidth, int newHeight) override {
            if (newWidth != width())
                updateDirectWriteStructures(newWidth);
            RendererWindow::resize(newWidth, newHeight);
        }

    protected:

        void windowResized(int width, int height) override{
            ASSERT(rt_ != nullptr);
            D2D1_SIZE_U size = D2D1::SizeU(width, height);
            rt_->Resize(size);
            RendererWindow::windowResized(width, height);
        }


        /** Destroys the renderer's window. 
         */
        void rendererClose() override {
            RendererWindow::rendererClose();
            DestroyWindow(hWnd_);
        }


        /** Enable mouse tracking so that the mouseOut event is properly reported. 
         */
        void rendererMouseIn() override {
            TRACKMOUSEEVENT tm;
            tm.cbSize = sizeof(tm);
            tm.dwFlags = TME_LEAVE;
            tm.hwndTrack = hWnd_;
            TrackMouseEvent(&tm);                    
            RendererWindow::rendererMouseIn();
        }

        /** Registers mouse button down.
          
            Starts mouse capture if no mouse button has been pressed previously, which allows the terminal window to track mouse movement outside of the window if at least one mouse button is pressed. 
         */ 
        void rendererMouseDown(Point coords, MouseButton button, Key modifiers) override {
            RendererWindow::rendererMouseDown(coords, button, modifiers);
            if (mouseButtonsDown_ == 1)
                SetCapture(hWnd_);
        }

        /** Registers mouse button up. 
          
            If there are no more pressed buttons left, releases the mouse capture previously obtained. 
         */
        void rendererMouseUp(Point coords, MouseButton button, Key modifiers) override {
            RendererWindow::rendererMouseUp(coords, button, modifiers);
            if (mouseButtonsDown_ == 0)
                ReleaseCapture();
        }

        /** Mouse moves. 
         
            Since we need to track mouse leave, checks whether the mouse leave event is currently tracked and if not, enables the tracking, which is disabled when the mouseOut event is emited. 
         */
        void rendererMouseMove(Point coords, Key modifiers) override {
            // enable tracking if not enabled
            if (! mouseLeaveTracked_) {
                TRACKMOUSEEVENT tm;
                tm.cbSize = sizeof(tm);
                tm.dwFlags = TME_LEAVE;
                tm.hwndTrack = hWnd_;
                mouseLeaveTracked_ = TrackMouseEvent(&tm);
                ASSERT(mouseLeaveTracked_);
            }
            RendererWindow::rendererMouseMove(coords, modifiers);
        }

        void requestClipboard(Widget * sender) override;

        void requestSelection(Widget * sender) override;

        void rendererSetClipboard(std::string const & contents) override;

        void rendererRegisterSelection(std::string const & contents, Widget * owner) override;

        void rendererClearSelection() override;

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
        void changeFont(ui::Font font) {
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
            // determine the originl and draw the glyph run
            D2D1_POINT_2F origin = D2D1::Point2F(
                static_cast<float>(glyphRunCol_* cellWidth_ + font_->offsetLeft()),
                ((glyphRunRow_ + 1 - state_.font().height()) * cellHeight_ + font_->ascent()) + font_->offsetTop());
            if (!state_.font().blink() || BlinkVisible_) {
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
        }

        /** Draws the borders of a single cell. 
         */
        void drawBorder(int col, int row, Border const & border, int widthThin, int widthThick) {
            float fLeft = static_cast<float>(col * cellWidth_);
            float fTop = static_cast<float>(row * cellHeight_);
            float widthTop = static_cast<float>(border.top() == Border::Kind::None ? 0 : (border.top() == Border::Kind::Thick ? widthThick : widthThin));
            float widthLeft = static_cast<float>(border.left() == Border::Kind::None ? 0 : (border.left() == Border::Kind::Thick ? widthThick : widthThin));
            float widthBottom = static_cast<float>(border.bottom() == Border::Kind::None ? 0 : (border.bottom() == Border::Kind::Thick ? widthThick : widthThin));
            float widthRight = static_cast<float>(border.right() == Border::Kind::None ? 0 : (border.right() == Border::Kind::Thick ? widthThick : widthThin));

            // start with top bar
            D2D1_RECT_F rect = D2D1::RectF(fLeft, fTop, fLeft + cellWidth_, fTop + widthTop);
            if (widthTop != 0)
                rt_->FillRectangle(rect, bg_.Get());
            // update the rectangle for bottom bar
            rect.bottom = fTop + cellHeight_;
            rect.top = rect.bottom - widthBottom;
            if (widthBottom != 0)
                rt_->FillRectangle(rect, bg_.Get());
            // and finally deal with the middle section (left and right)
            rect.bottom = rect.top;
            rect.top = fTop + widthTop;
            if (widthLeft != 0) {
                rect.right = fLeft + widthLeft;
                rt_->FillRectangle(rect, bg_.Get());
            }
            if (widthRight != 0) {
                rect.right = fLeft + cellWidth_;
                rect.left = rect.right - widthRight;
                rt_->FillRectangle(rect, bg_.Get());
            }
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

        /** Determines if the mouse leaving the window are tracked, since there is no WM_MOUSEENTER message in Win32. */
        bool mouseLeaveTracked_;

	    static Key GetKey(unsigned vk);

        static LRESULT CALLBACK EventHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

    }; // tpp::DirectWriteWindow


} // namespace tpp

#endif
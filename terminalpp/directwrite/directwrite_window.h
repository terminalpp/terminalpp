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

        void setTitle(std::string const & value) override;

        void setIcon(Window::Icon icon) override;

        void setFullscreen(bool value = true) override;

        void show(bool value = true) override {
            if (value)
                ShowWindow(hWnd_, SW_SHOWNORMAL);
            else
                ShowWindow(hWnd_, SW_HIDE);
        }

        void resize(Size const & newSize) override {
            if (newSize.width() != width())
                updateDirectWriteStructures(newSize.width());
            RendererWindow::resize(newSize);
        }

        void close() override {
            RendererWindow::close();
            DestroyWindow(hWnd_);
        }

        /** Schedules the event and notifies the main thread that an event is ready. 
         */
        void schedule(std::function<void()> event, Widget * widget) override {
            RendererWindow::schedule(event, widget);
            PostMessage(DirectWriteApplication::Instance()->dummy_, WM_USER, 0, 0);
        }

    protected:

        void windowResized(int width, int height) override{
            ASSERT(rt_ != nullptr);
            D2D1_SIZE_U size = D2D1::SizeU(width, height);
            rt_->Resize(size);

            // redo the window's window resized here as we need to adjust the col & row size based on the dpiFactor
            if (width != sizePx_.width() || height != sizePx_.height()) {
                sizePx_ = Size{width, height};
                // tell the renderer to resize 
                resize(Size{
                    static_cast<int>(width / dpiFactor_ / cellSize_.width()), 
                    static_cast<int>(height / dpiFactor_ / cellSize_.height())
                });
            }
        }

        Point pixelsToCoords(Point xy) {
            return RendererWindow::pixelsToCoords(xy / dpiFactor_);
        }

        /** Sets the cursor. 
         
            Win32 does not support distinction between splitter and resizer. 
         */
        void setMouseCursor(MouseCursor cursor) override {
            switch (cursor)  {
                case MouseCursor::Default:
                case MouseCursor::Beam:
                default:
                    mouseCursor_ = DirectWriteApplication::Instance()->cursorBeam_;
                    break;
                case MouseCursor::Arrow:
                    mouseCursor_ = DirectWriteApplication::Instance()->cursorArrow_;
                    break;
                case MouseCursor::Hand:
                    mouseCursor_ = DirectWriteApplication::Instance()->cursorHand_;
                    break;
                case MouseCursor::VerticalSize:
                case MouseCursor::VerticalSplit:
                    mouseCursor_ = DirectWriteApplication::Instance()->cursorVerticalSize_;
                    break;
                case MouseCursor::HorizontalSize:
                case MouseCursor::HorizontalSplit:
                    mouseCursor_ = DirectWriteApplication::Instance()->cursorHorizontalSize_;
                    break;
                case MouseCursor::Wait:
                    mouseCursor_ = DirectWriteApplication::Instance()->cursorWait_;
                    break;
                case MouseCursor::Forbidden:
                    mouseCursor_ = DirectWriteApplication::Instance()->cursorForbidden_;
                    break;
            }
            SetCursor(mouseCursor_);
        }

        /** Enable mouse tracking so that the mouseOut event is properly reported. 
         */
        void mouseIn() override {
            TRACKMOUSEEVENT tm;
            tm.cbSize = sizeof(tm);
            tm.dwFlags = TME_LEAVE;
            tm.hwndTrack = hWnd_;
            TrackMouseEvent(&tm);                    
            RendererWindow::mouseIn();
        }

        /** Triggered when mouse leaves the window. 
         
            Since there are spurious double WM_MOUSELEAVE messages sent to the window in certain cases (such as when wheel message is received *before* mouse move), all but the first mouse out invocation after mouse capture are stopped. 
         */
        void mouseOut() override {
            // stop spurious mouse leaves when we do not expect them
            if (! mouseLeaveTracked_)
                return;
            mouseLeaveTracked_ = false;
            RendererWindow::mouseOut();
        }

        /** Registers mouse button down.
          
            Starts mouse capture if no mouse button has been pressed previously, which allows the terminal window to track mouse movement outside of the window if at least one mouse button is pressed. 
         */ 
        void mouseDown(Point coords, MouseButton button) override {
            RendererWindow::mouseDown(coords, button);
            if (mouseButtonsDown_ == 1)
                SetCapture(hWnd_);
        }

        /** Registers mouse button up. 
          
            If there are no more pressed buttons left, releases the mouse capture previously obtained. 
         */
        void mouseUp(Point coords, MouseButton button) override {
            RendererWindow::mouseUp(coords, button);
            if (mouseButtonsDown_ == 0)
                ReleaseCapture();
        }

        /** Mouse moves. 
         
            Since we need to track mouse leave, checks whether the mouse leave event is currently tracked and if not, enables the tracking, which is disabled when the mouseOut event is emited. 
         */
        void mouseMove(Point coords) override {
            // emit mouse in first if necessary (there is no WM_MOUSEENTER)
            if (!rendererMouseCaptured())
                mouseIn();
            // enable tracking if not enabled
            if (! mouseLeaveTracked_) {
                TRACKMOUSEEVENT tm;
                tm.cbSize = sizeof(tm);
                tm.dwFlags = TME_LEAVE;
                tm.hwndTrack = hWnd_;
                mouseLeaveTracked_ = TrackMouseEvent(&tm);
                ASSERT(mouseLeaveTracked_);
            }
            RendererWindow::mouseMove(coords);
        }

        /** Mouse wheel movement. 
         
            Mouse wheel can event can be received *before* mouse move when the mouse scrolls and enters the window, if that is the case triggers the mouseMove event first. 
         */
        void mouseWheel(Point coords, int by) override {
            if (!rendererMouseCaptured())
                mouseMove(coords);
            RendererWindow::mouseWheel(coords, by);
        }

        void requestClipboard(Widget * sender) override;

        void requestSelection(Widget * sender) override;

        void setClipboard(std::string const & contents) override;

        void setSelection(std::string const & contents, Widget * owner) override;

        void clearSelection(Widget * sender) override;

    private:

        friend class DirectWriteApplication;

        friend class RendererWindow<DirectWriteWindow, HWND>;

        /** Creates the renderer window of appropriate size using the default font and zoom of 1.0. 
         
            TODO in the future, I want the zoom to be configurable. 
         */
        DirectWriteWindow(std::string const & title, int width, int height, EventQueue & eventQueue);

        /** \name Rendering Functions
         */
        //@{

        void initializeDraw() {
            rt_->BeginDraw();
        }

        void finalizeDraw() {
            changeBackgroundColor(backgroundColor());
            if (sizePx_.width() % cellSize_.width() != 0) {
                D2D1_RECT_F rect = D2D1::RectF(
                    static_cast<FLOAT>(width() * cellSize_.width()),
                    static_cast<FLOAT>(0),
                    static_cast<FLOAT>(sizePx_.width()),
                    static_cast<FLOAT>(sizePx_.height())
                );
    			rt_->FillRectangle(rect, bg_.Get());
            }
            if (sizePx_.height() % cellSize_.height() != 0) {
                D2D1_RECT_F rect = D2D1::RectF(
                    static_cast<FLOAT>(0),
                    static_cast<FLOAT>(height() * cellSize_.height()),
                    static_cast<FLOAT>(sizePx_.width()),
                    static_cast<FLOAT>(sizePx_.height())
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
                const_cast<float *>(glyphRun_.glyphAdvances)[glyphRun_.glyphCount] = static_cast<float>(cellSize_.width() * font_->font().width());
                font_->fontFace()->GetGlyphIndices(&cp, 1, glyphIndices_);
                glyphRun_.glyphCount = 1;
                drawGlyphRun();
                // revert the font back to what we had before and reinitialize the glyphrun to start at next character
                font_ = oldFont;
                initializeGlyphRun(col + font_->font().width(), row);
                glyphRun_.fontFace = font_->fontFace();
                glyphRun_.fontEmSize = font_->sizeEm();
            } else {
                const_cast<float *>(glyphRun_.glyphAdvances)[glyphRun_.glyphCount] = static_cast<float>(cellSize_.width() * font_->font().width());
                ++glyphRun_.glyphCount;
            }
        }

        /** Updates the current font.
         */
        void changeFont(ui::Font font) {
			font_ = DirectWriteFont::Get(font, cellSize_);
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
				static_cast<FLOAT>(glyphRunCol_ * cellSize_.width()),
				static_cast<FLOAT>((glyphRunRow_ + 1 - state_.font().height()) * cellSize_.height()),
				static_cast<FLOAT>((glyphRunCol_ + glyphRun_.glyphCount * state_.font().width()) * cellSize_.width()),
				static_cast<FLOAT>((glyphRunRow_ + 1) * cellSize_.height())
			);
            // fill it with the background
			rt_->FillRectangle(rect, bg_.Get());
            // determine the originl and draw the glyph run
            D2D1_POINT_2F origin = D2D1::Point2F(
                static_cast<float>(glyphRunCol_* cellSize_.width() + font_->offset().x()),
                ((glyphRunRow_ + 1 - state_.font().height()) * cellSize_.height() + font_->ascent()) + font_->offset().y());
            if (!state_.font().blink() || BlinkVisible()) {
                rt_->DrawGlyphRun(origin, &glyphRun_, fg_.Get());
                // see if there are any attributes to be drawn 
                if (state_.font().underline()) {
                    D2D1_POINT_2F start = origin;
                    start.y -= font_->underlineOffset();
                    D2D1_POINT_2F end = start;
                    end.x += glyphRun_.glyphCount * cellSize_.width();
                    rt_->DrawLine(start, end, decor_.Get(), font_->underlineThickness(), state_.font().dashed() ? DirectWriteApplication::Instance()->dashedStroke_ : nullptr);
                }
                if (state_.font().strikethrough()) {
                    D2D1_POINT_2F start = origin;
                    start.y -= font_->strikethroughOffset();
                    D2D1_POINT_2F end = start;
                    end.x += glyphRun_.glyphCount * cellSize_.width();
                    rt_->DrawLine(start, end, decor_.Get(), font_->strikethroughThickness(), state_.font().dashed() ? DirectWriteApplication::Instance()->dashedStroke_ : nullptr);
                }
            }
        }

        /** Draws the borders of a single cell. 
         */
        void drawBorder(int col, int row, Border const & border, int widthThin, int widthThick) {
            float fLeft = static_cast<float>(col * cellSize_.width());
            float fTop = static_cast<float>(row * cellSize_.height());
            float widthTop = static_cast<float>(border.top() == Border::Kind::None ? 0 : (border.top() == Border::Kind::Thick ? widthThick : widthThin));
            float widthLeft = static_cast<float>(border.left() == Border::Kind::None ? 0 : (border.left() == Border::Kind::Thick ? widthThick : widthThin));
            float widthBottom = static_cast<float>(border.bottom() == Border::Kind::None ? 0 : (border.bottom() == Border::Kind::Thick ? widthThick : widthThin));
            float widthRight = static_cast<float>(border.right() == Border::Kind::None ? 0 : (border.right() == Border::Kind::Thick ? widthThick : widthThin));

            // start with top bar
            D2D1_RECT_F rect = D2D1::RectF(fLeft, fTop, fLeft + cellSize_.width(), fTop + widthTop);
            if (widthTop != 0)
                rt_->FillRectangle(rect, bg_.Get());
            // update the rectangle for bottom bar
            rect.bottom = fTop + cellSize_.height();
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
                rect.right = fLeft + cellSize_.width();
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

        /** The DPI factor of the window so that different dpi levels can be used. (the base cell size has to be multipled by this to determine the real cell size. Real numbers are used to match for subpixel rendering)
         */
        double dpiFactor_;

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

        /** Currently set mouse cursor so that it can be updated on each mouse move as required by Win32. 
         */
        HCURSOR mouseCursor_;

	    static Key GetKey(unsigned vk);

        static LRESULT CALLBACK EventHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

    }; // tpp::DirectWriteWindow


} // namespace tpp

#endif
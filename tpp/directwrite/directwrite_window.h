#pragma once
#if (defined ARCH_WINDOWS)

#include <unordered_map>

#include "helpers/time.h"

#include "../window.h"

#include "../font.h"
#include "directwrite_font.h"

namespace tpp {

    class DirectWriteWindow : public RendererWindow<DirectWriteWindow> {
    public:

		typedef Font<DirectWriteFont> Font;

        ~DirectWriteWindow() override;

        void show() override {
            ShowWindow(hWnd_, SW_SHOWNORMAL);
        }

        void hide() override {
            NOT_IMPLEMENTED;
        }

        void close() override {
            PostMessage(hWnd_, WM_CLOSE, 0, 0);
        }

        /** Schedules the window to be repainted.

            Instead of invalidating the rectange, WM_PAINT must explicitly be sent, as it may happen that different thread is already repainting the window, and therefore the request will be silenced (the window region is validated at the end of WM_PAINT). 
         */
		void paint(ui::RectEvent & e) override {
            MARK_AS_UNUSED(e);
            PostMessage(hWnd_, WM_PAINT, 0, 0);
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

    private:
        friend class DirectWriteApplication;
        friend class RendererWindow<DirectWriteWindow>;

        /** Updates the glyph run structures so that up to an entire line can be fit in a single glyph run. 
         */
        void updateDirectWriteStructures(int cols);

        // rendering methods - we really want these to inline

        void initializeDraw() {
            rt_->BeginDraw();
        }

        void finalizeDraw() {
            rt_->EndDraw();
        }

        bool shouldDraw(int col, int row, ui::Cell const & cell) {
            MARK_AS_UNUSED(col);
            MARK_AS_UNUSED(row);
            MARK_AS_UNUSED(cell);
            // draw all by default
            return true;
        }

        void initializeGlyphRun(int col, int row) {
            glyphRun_.glyphCount = 0;
            glyphRunCol_ = col;
            glyphRunRow_ = row;
        }

        void addGlyph(ui::Cell const & cell) {
            UINT32 cp = cell.codepoint();
            dwFont_->nativeHandle().fontFace->GetGlyphIndices(&cp, 1, glyphIndices_ + glyphRun_.glyphCount);
            const_cast<float *>(glyphRun_.glyphAdvances)[glyphRun_.glyphCount] = static_cast<float>(dwFont_->cellWidthPx());
            ++glyphRun_.glyphCount;
        }

        /** Updates the current font.
         
            If there is non-empty glyph run, draws it before changing the font. 
         */
        void setFont(ui::Font font) {
			dwFont_ = Font::GetOrCreate(font, cellHeightPx_);
			glyphRun_.fontFace = dwFont_->nativeHandle().fontFace.Get();
			glyphRun_.fontEmSize = dwFont_->nativeHandle().sizeEm;
        }

        /** Updates the foreground color.
         
            If there is non-empty glyph run, draws it before changing the color. 
         */
        void setForegroundColor(ui::Color color) {
			fg_->SetColor(D2D1::ColorF(D2D1::ColorF(color.toRGB(), color.floatAlpha())));
        }

        /** Updates the background color. 
         
            If there is non-empty glyph run, draws it before changing the color.
         */
        void setBackgroundColor(ui::Color color) {
		    bg_->SetColor(D2D1::ColorF(D2D1::ColorF(color.toRGB(), color.floatAlpha())));
        }

        /** Updates the decoration color. 
         
            If there is non-empty glyph run, draws it before changint the color.
         */
        void setDecorationColor(ui::Color color) {
            decor_->SetColor(D2D1::ColorF(D2D1::ColorF(color.toRGB(), color.floatAlpha())));
        }

        /** Sets the attributes of the cell. 
         */
        void setAttributes(ui::Attributes const & attrs) {
            attrs_ = attrs;
        }

        /** Draws the glyph run. 
         
            Should only be really called if there are glyphs to run. First clears the background with given background color, then draws the text and finally applies any decorations. 
         */
        void drawGlyphRun() {
            if (glyphRun_.glyphCount == 0)
                return;
            // get the glyph run rectange
			D2D1_RECT_F rect = D2D1::RectF(
				static_cast<FLOAT>(glyphRunCol_ * cellWidthPx_),
				static_cast<FLOAT>(glyphRunRow_ * cellHeightPx_),
				static_cast<FLOAT>((glyphRunCol_ + glyphRun_.glyphCount) * cellWidthPx_),
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
                static_cast<float>(glyphRunCol_* cellWidthPx_),
                static_cast<float>(glyphRunRow_ * cellHeightPx_ + dwFont_->nativeHandle().ascent));
            rt_->DrawGlyphRun(origin, &glyphRun_, fg_.Get());
            // see if there are any attributes to be drawn 
            if (!attrs_.emptyDecorations()) {
                if (attrs_.underline()) {
					D2D1_POINT_2F start = origin;
					start.y -= dwFont_->nativeHandle().underlineOffset;
					D2D1_POINT_2F end = start;
					end.x += glyphRun_.glyphCount * cellWidthPx_;
					rt_->DrawLine(start, end, decor_.Get(), dwFont_->nativeHandle().underlineThickness);
                }
                if (attrs_.strikethrough()) {
					D2D1_POINT_2F start = origin;
					start.y -= dwFont_->nativeHandle().strikethroughOffset;
					D2D1_POINT_2F end = start;
					end.x += glyphRun_.glyphCount * cellWidthPx_;
					rt_->DrawLine(start, end, decor_.Get(), dwFont_->nativeHandle().strikethroughThickness);
                }
            }
			glyphRun_.glyphCount = 0;
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
		Font* dwFont_;
        ui::Attributes attrs_;

		DWRITE_GLYPH_RUN glyphRun_;
		UINT16 * glyphIndices_;
		FLOAT * glyphAdvances_;
		DWRITE_GLYPH_OFFSET * glyphOffsets_;
		int glyphRunCol_;
		int glyphRunRow_;



        static std::unordered_map<HWND, DirectWriteWindow *> Windows_;

        static DirectWriteWindow * GetWindowFromHWND(HWND hWnd) {
            auto i = Windows_.find(hWnd);
            return i == Windows_.end() ? nullptr : i->second;
        }

        static ui::Key GetKey(unsigned vk);

        static LRESULT CALLBACK EventHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);


    };


} // namespace tpp



#endif
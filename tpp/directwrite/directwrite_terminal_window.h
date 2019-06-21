#pragma once
#ifdef WIN32

#include <unordered_map>

#include <windows.h>
#include <d2d1_2.h>
#include <dwrite_2.h>

#include "helpers/log.h"

#include "../terminal_window.h"

#include "directwrite_application.h"

namespace tpp {

	class DirectWriteApplication;

	template<>
	inline FontSpec<DWriteFont> * FontSpec<DWriteFont>::Create(vterm::Font font, unsigned baseHeight) {
		// get the system font collection		
		Microsoft::WRL::ComPtr<IDWriteFontCollection> sfc;
		Application::Instance<DirectWriteApplication>()->dwFactory_->GetSystemFontCollection(&sfc, false);
		// find the required font family - first get the index then obtain the family by the index
		UINT32 findex;
		BOOL fexists;
		sfc->FindFamilyName(L"Iosevka Term", &findex, &fexists);
		Microsoft::WRL::ComPtr<IDWriteFontFamily> ff;
		sfc->GetFontFamily(findex, &ff);
		// now get the nearest font
		Microsoft::WRL::ComPtr<IDWriteFont> drw;
		ff->GetFirstMatchingFont(
			font.bold() ? DWRITE_FONT_WEIGHT_BOLD : DWRITE_FONT_WEIGHT_REGULAR,
			DWRITE_FONT_STRETCH_NORMAL,
			font.italics() ? DWRITE_FONT_STYLE_ITALIC : DWRITE_FONT_STYLE_NORMAL,
			&drw);
		// finally get the font face
		Microsoft::WRL::ComPtr<IDWriteFontFace> fface;
		drw->CreateFontFace(&fface);
		// now we need to determine the dimensions of single character, which is relatively involved operation, so first we get the dpi and font metrics
		FLOAT dpiX;
		FLOAT dpiY;
		Application::Instance<DirectWriteApplication>()->d2dFactory_->GetDesktopDpi(&dpiX, &dpiY);
		DWRITE_FONT_METRICS metrics;
		fface->GetMetrics(&metrics);
		// the em size is size in pixels divided by (DPI / 96)
		// https://docs.microsoft.com/en-us/windows/desktop/LearnWin32/dpi-and-device-independent-pixels
		double emSize = (baseHeight * font.size()) / (dpiY / 96);
		// we have to adjust this number for the actual font metrics
		emSize = emSize * metrics.designUnitsPerEm / (metrics.ascent + metrics.descent + metrics.lineGap);
		// now we have to determine the height of a character, which we can do via glyph metrics
		DWRITE_GLYPH_METRICS glyphMetrics;
		UINT16 glyph;
		UINT32 codepoint = 'M';
		fface->GetGlyphIndicesA(&codepoint, 1, &glyph);
		fface->GetDesignGlyphMetrics(&glyph, 1, &glyphMetrics);

		return new FontSpec<DWriteFont>(font,
			std::round((static_cast<double>(glyphMetrics.advanceWidth) / glyphMetrics.advanceHeight) * baseHeight * font.size()),
			baseHeight * font.size(),
			DWriteFont(
				fface,
				emSize,
				std::round(emSize * metrics.ascent / metrics.designUnitsPerEm)));
	}

	/** Since DirectWrite stores only the font face and the glyph run then keeps its own size. 
	 */
	template<>
	inline vterm::Font FontSpec<DWriteFont>::StripEffects(vterm::Font const & font) {
		vterm::Font result(font);
		result.setBlink(false);
		result.setStrikeout(false);
		result.setUnderline(false);
		return result;
	}

	class DirectWriteTerminalWindow : public TerminalWindow {
	public:
		typedef FontSpec<DWriteFont> Font;

		DirectWriteTerminalWindow(Session* session, Properties const& properties, std::string const& title);

		void show() override {
			ShowWindow(hWnd_, SW_SHOWNORMAL);
		}

		void hide() override {
			NOT_IMPLEMENTED;
		}

		void close() override {
			LOG << "Closing window " << title_;
			PostMessage(hWnd_, WM_CLOSE, 0, 0);
		}

		void inputReady() override {
			PostMessage(hWnd_, WM_USER, MSG_INPUT_READY, 0);
		}

	protected:

		/** Returns the Application singleton, converted to GDIApplication.
		 */
		DirectWriteApplication* app() {
			return reinterpret_cast<DirectWriteApplication*>(Application::Instance());
		}

		/** Just deleting a terminal window is not allowed, therefore protected.
		 */
		~DirectWriteTerminalWindow() override;

		void doSetFullscreen(bool value) override;

		void titleChange(vterm::Terminal::TitleChangeEvent& e) override;

		void clipboardUpdated(vterm::Terminal::ClipboardUpdateEvent& e) override;

		void windowResized(unsigned widthPx, unsigned heightPx) override {
			if (rt_ != nullptr) {
				D2D1_SIZE_U size = D2D1::SizeU(widthPx, heightPx);
				rt_->Resize(size);
				updateGlyphRunStructures(widthPx, cellWidthPx_);
			}
			TerminalWindow::windowResized(widthPx, heightPx);
		}

		void doSetZoom(double value) override {
			TerminalWindow::doSetZoom(value);
			updateGlyphRunStructures(widthPx_, cellWidthPx_);
		}

		void updateGlyphRunStructures(unsigned width, unsigned fontWidth) {
			delete[] glyphIndices_;
			delete[] glyphAdvances_;
			delete[] glyphOffsets_;
			unsigned cols = width / fontWidth;
			glyphIndices_ = new UINT16[cols];
			glyphAdvances_ = new FLOAT[cols];
			glyphOffsets_ = new DWRITE_GLYPH_OFFSET[cols];
			glyphRun_.glyphIndices = glyphIndices_;
			glyphRun_.glyphAdvances = glyphAdvances_;
			glyphRun_.glyphOffsets = glyphOffsets_;
			ZeroMemory(glyphOffsets_, sizeof(DWRITE_GLYPH_OFFSET) * cols);
			for (size_t i = 0; i < cols; ++i)
				glyphAdvances_[i] = fontWidth;
			glyphRun_.glyphCount = 0;
			doSetFont(vterm::Font());
		}

		/** Deletes the double buffer object.
		 */
		void doInvalidate(bool forceRepaint) override {
			// set the invalidate flag
			TerminalWindow::doInvalidate(forceRepaint);
			// repaint the window
			InvalidateRect(hWnd_, /* rect */ nullptr, /* erase */ false);
		}

		void clipboardPaste() override;

		unsigned doPaint() override;

		void doSetForeground(vterm::Color const& fg) override {
			drawGlyphRun();
			fg_->SetColor(D2D1::ColorF(D2D1::ColorF(fg.toNumber(), 1.0f)));
		}

		void doSetBackground(vterm::Color const& bg) override {
			drawGlyphRun();
			bg_->SetColor(D2D1::ColorF(D2D1::ColorF(bg.toNumber(), 1.0f)));
		}

		void doSetFont(vterm::Font font) override {
			drawGlyphRun();
			dwFont_ = Font::GetOrCreate(font, cellHeightPx_);
			glyphRun_.fontFace = dwFont_->handle().fontFace.Get();
			glyphRun_.fontEmSize = dwFont_->handle().sizeEm;
		}

		void doDrawCell(unsigned col, unsigned row, vterm::Terminal::Cell const& c) override {
			if (glyphRun_.glyphCount != 0 && (col != glyphRunCol_ + glyphRun_.glyphCount || row != glyphRunRow_)) {
				drawGlyphRun();
			}
			if (glyphRun_.glyphCount == 0) {
				glyphRunCol_ = col;
				glyphRunRow_ = row;
			} 
			UINT32 cp = c.c.codepoint();
			dwFont_->handle().fontFace->GetGlyphIndicesA(&cp, 1, glyphIndices_ + glyphRun_.glyphCount);
			++glyphRun_.glyphCount;
		}

		void doDrawCursor(unsigned col, unsigned row, vterm::Terminal::Cell const& c) override {
			drawGlyphRun();
			doSetForeground(c.fg);
			doSetFont(c.font);
			bg_->SetOpacity(0);
			doDrawCell(col, row, c);
			drawGlyphRun();
			bg_->SetOpacity(1);
		}

		void doClearWindow() override {
			bg_->SetColor(D2D1::ColorF(0, 1.0f));
			rt_->Clear(D2D1::ColorF(D2D1::ColorF::Black));
		}

		void drawGlyphRun() {
			if (glyphRun_.glyphCount == 0)
				return;
			D2D1_RECT_F rect = D2D1::RectF(
				static_cast<FLOAT>(glyphRunCol_ * cellWidthPx_),
				static_cast<FLOAT>(glyphRunRow_ * cellHeightPx_),
				static_cast<FLOAT>((glyphRunCol_ + glyphRun_.glyphCount) * cellWidthPx_),
				static_cast<FLOAT>((glyphRunRow_ + 1) * cellHeightPx_)
			);
			rt_->FillRectangle(rect, bg_.Get());
			D2D1_POINT_2F origin = D2D1::Point2F((glyphRunCol_) * cellWidthPx_, glyphRunRow_ * cellHeightPx_ + dwFont_->handle().ascent);
			rt_->DrawGlyphRun(origin, &glyphRun_, fg_.Get());
			glyphRun_.glyphCount = 0;
		}

	private:

		static constexpr WPARAM MSG_TITLE_CHANGE = 0;

		/** New input is available for the attached backend and should be processed in the main event loop.
		 */
		static constexpr WPARAM MSG_INPUT_READY = 1;

		friend class DirectWriteApplication;

		/** Maps win32 virtual keys to their vterm equivalents.
		 */
		static vterm::Key GetKey(WPARAM vk);

		static LRESULT CALLBACK EventHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

		HWND hWnd_;

		/** Direct X structures. 
		 */
		Microsoft::WRL::ComPtr<ID2D1HwndRenderTarget> rt_;
		Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> fg_; // foreground (text) style
		Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> bg_; // background style
		DWRITE_GLYPH_RUN glyphRun_;
		UINT16 * glyphIndices_;
		FLOAT * glyphAdvances_;
		DWRITE_GLYPH_OFFSET * glyphOffsets_;
		unsigned glyphRunCol_;
		unsigned glyphRunRow_;

		Font* dwFont_;

		/** Placement of the window to which the window is restored after fullscreen toggle.
		 */
		WINDOWPLACEMENT wndPlacement_;

		/** Width and height of the window frame so that the width and height of the window can be adjusted accordingly.
		 */
		unsigned frameWidth_;
		unsigned frameHeight_;

		static std::unordered_map<HWND, DirectWriteTerminalWindow*> Windows_;

		static constexpr UINT_PTR TIMERID_BLINK = 34;


	};

} // namespace tpp
#endif
#pragma once
#ifdef WIN32

#include <unordered_map>

#include <windows.h>
#include <d2d1_2.h>
#include <dwrite_2.h>

#include "helpers/log.h"

#include "../terminal_window.h"

namespace tpp {

	// GetSystemFontCollection -> GetFontFamily -> GetMatchingFont -> CreateFont face
	// use the com pointers - see tutorial or so

	class DirectWriteApplication;

	template<>
	inline FontSpec<IDWriteTextFormat*>* FontSpec<IDWriteTextFormat*>::Create(vterm::Font font, unsigned baseHeight) {
		IDWriteTextFormat* tf;
		DirectWriteApplication* app = reinterpret_cast<DirectWriteApplication*>(Application::Instance());
		// TODO figure dip 
		OSCHECK(SUCCEEDED(app->dwFactory_->CreateTextFormat(
			L"Iosevka Term",  // Font family name.
			NULL, // Font collection (NULL sets it to use the system font collection).
			font.bold() ? DWRITE_FONT_WEIGHT_BOLD : DWRITE_FONT_WEIGHT_REGULAR,
			font.italics() ? DWRITE_FONT_STYLE_ITALIC : DWRITE_FONT_STYLE_NORMAL,
			DWRITE_FONT_STRETCH_NORMAL,
			baseHeight * font.size() - 3.6,
			L"en-us",
			&tf
		))) << "Unable to create font";
		IDWriteTextLayout * tl;
		app->dwFactory_->CreateTextLayout(L"M", 1, tf, 1000, 1000, &tl);
		DWRITE_TEXT_METRICS tm;
		tl->GetMetrics(&tm);
		tl->Release();
		// TODO the
		return new FontSpec<IDWriteTextFormat*>(font, std::round(tm.width), std::round(tm.height), tf);
	}

	class DirectWriteTerminalWindow : public TerminalWindow {
	public:
		typedef FontSpec<IDWriteTextFormat*> Font;

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

		void windowResized(unsigned widthPx, unsigned heightPx) override {
			if (rt_ != nullptr) {
				D2D1_SIZE_U size = D2D1::SizeU(widthPx, heightPx);
				rt_->Resize(size);
				glyphRun_.glyphIndices = &glyphIndices_;
				glyphRun_.glyphAdvances = &glyphAdvances_;
				glyphRun_.glyphOffsets = &glyphOffsets_;
				glyphAdvances_ = 0;
				glyphOffsets_.advanceOffset = 0;
				glyphOffsets_.ascenderOffset = 0;
			}
			TerminalWindow::windowResized(widthPx, heightPx);
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
		void clipboardCopy(std::string const& str) override;

		unsigned doPaint() override;

		void doSetForeground(vterm::Color const& fg) override {
			fg_->SetColor(D2D1::ColorF(D2D1::ColorF(fg.toNumber(), 1.0f)));
		}

		void doSetBackground(vterm::Color const& bg) override {
			bg_->SetColor(D2D1::ColorF(D2D1::ColorF(bg.toNumber(), 1.0f)));
		}

		void doSetFont(vterm::Font font) override {
			font_ = Font::GetOrCreate(font, cellHeightPx_);
		}

		void doDrawCell(unsigned col, unsigned row, vterm::Terminal::Cell const& c) override {
			D2D1_RECT_F rect = D2D1::RectF(
				static_cast<FLOAT>(col * cellWidthPx_),
				static_cast<FLOAT>(row * cellHeightPx_),
				static_cast<FLOAT>((col + 1) * cellWidthPx_),
				static_cast<FLOAT>((row + 1) * cellHeightPx_)
			);
			UINT32 cp = c.c.codepoint();
			fface_->GetGlyphIndicesA(&cp, 1, &glyphIndices_);
			rt_->FillRectangle(rect, bg_.Get());
			D2D1_POINT_2F origin = D2D1::Point2F((col + 1) * cellWidthPx_, (row + 1) * cellHeightPx_ - 4);
			rt_->DrawGlyphRun(origin, &glyphRun_, fg_.Get());
		}

		void doDrawCursor(unsigned col, unsigned row, vterm::Terminal::Cell const& c) override {
			doSetForeground(c.fg);
			doSetFont(c.font);
			bg_->SetOpacity(0);
			doDrawCell(col, row, c);
			bg_->SetOpacity(1);
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
		UINT16 glyphIndices_;
		FLOAT glyphAdvances_;
		DWRITE_GLYPH_OFFSET glyphOffsets_;
		Microsoft::WRL::ComPtr<IDWriteFontFace> fface_;

		Font* font_;

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
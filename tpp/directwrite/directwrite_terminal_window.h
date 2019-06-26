#pragma once
#ifdef WIN32

#include <unordered_map>

#include <windows.h>
#include <d2d1_2.h>
#include <dwrite_2.h>

#include "helpers/log.h"

#include "../config.h"
#include "../terminal_window.h"

#include "directwrite_application.h"

namespace tpp {

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
			PostMessage(hWnd_, WM_USER, DirectWriteApplication::MSG_INPUT_READY, 0);
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
			grBlink_ = font.blink();
			grUnderline_ = font.underline();
			grStrikethrough_ = font.strikethrough();
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
			// if blinking, only draw when blink is on
			if (!grBlink_ || blink_) {
				D2D1_POINT_2F origin = D2D1::Point2F((glyphRunCol_)* cellWidthPx_, glyphRunRow_ * cellHeightPx_ + dwFont_->handle().ascent);
				rt_->DrawGlyphRun(origin, &glyphRun_, fg_.Get());
				// add underline and strikethrough if selected, the position and thickness is obtained from the font metrics generated when the font is created (directwrite_application.h)
				if (grUnderline_) {
					D2D1_POINT_2F start = origin;
					start.y -= dwFont_->handle().underlineOffset;
					D2D1_POINT_2F end = start;
					end.x += glyphRun_.glyphCount * cellWidthPx_;
					rt_->DrawLine(start, end, fg_.Get(), dwFont_->handle().underlineThickness);
				}
				if (grStrikethrough_) {
					D2D1_POINT_2F start = origin;
					start.y -= dwFont_->handle().strikethroughOffset;
					D2D1_POINT_2F end = start;
					end.x += glyphRun_.glyphCount * cellWidthPx_;
					rt_->DrawLine(start, end, fg_.Get(), dwFont_->handle().strikethroughThickness);
				}
			}
			glyphRun_.glyphCount = 0;
		}

	private:

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

		/** Determines the font properties of current glyph run. 
		 */
		bool grBlink_;
		bool grUnderline_;
		bool grStrikethrough_;


		static std::unordered_map<HWND, DirectWriteTerminalWindow*> Windows_;

		static void BlinkTimer() {
			for (auto i : Windows_)
				i.second->blinkTimer();
		}

	};

} // namespace tpp
#endif
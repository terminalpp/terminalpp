#pragma once
#ifdef WIN32

#include <unordered_map>

#include <windows.h>
#include <d2d1_2.h>
#include <dwrite_2.h>

#include "helpers/log.h"

#include "../terminal_window.h"

namespace tpp {

	class DirectWriteApplication;

	template<>
	inline FontSpec<IDWriteTextFormat*>* FontSpec<IDWriteTextFormat*>::Create(vterm::Font font, unsigned height) {
		IDWriteTextFormat* tf;
		DirectWriteApplication* app = reinterpret_cast<DirectWriteApplication*>(Application::Instance());
		// TODO figure dip 
		OSCHECK(SUCCEEDED(app->dwFactory_->CreateTextFormat(
			L"Iosevka Term",  // Font family name.
			NULL, // Font collection (NULL sets it to use the system font collection).
			font.bold() ? DWRITE_FONT_WEIGHT_BOLD : DWRITE_FONT_WEIGHT_REGULAR,
			font.italics() ? DWRITE_FONT_STYLE_ITALIC : DWRITE_FONT_STYLE_NORMAL,
			DWRITE_FONT_STRETCH_NORMAL,
			height * font.size() - 3,
			L"en-us",
			&tf
		))) << "Unable to create font";
		// TODO figure the font width here
		return new FontSpec<IDWriteTextFormat*>(font, 8, height, tf);
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

		void redraw() override {
			/*
			if (buffer_ != nullptr) {
				DeleteObject(buffer_);
				buffer_ = nullptr;
			}
			*/
			InvalidateRect(hWnd_, /* rect */ nullptr, /* erase */ false);
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

		/** Deletes the double buffer object.
		 */
		void doInvalidate() override {
			// set the invalidate flag
			TerminalWindow::doInvalidate();
			// repaint the window
			InvalidateRect(hWnd_, /* rect */ nullptr, /* erase */ false);
		}

		void clipboardPaste() override;
		void clipboardCopy(std::string const& str) override;

		void doPaint() override;

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
			wchar_t wc = c.c.toWChar();
			D2D1_RECT_F rect = D2D1::RectF(
				static_cast<FLOAT>(col * cellWidthPx_),
				static_cast<FLOAT>(row * cellHeightPx_),
				static_cast<FLOAT>((col + 1) * cellWidthPx_),
				static_cast<FLOAT>((row + 1) * cellHeightPx_)
			);
			rt_->FillRectangle(rect, bg_);
			rt_->DrawText(
				&wc,
				1,
				font_->handle(),
				rect,
				fg_
			);
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

		//void updateBuffer();

		/** Maps win32 virtual keys to their vterm equivalents.
		 */
		static vterm::Key GetKey(WPARAM vk);

		static LRESULT CALLBACK EventHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

		HWND hWnd_;

		/** Direct X structures. 
		 */
		ID2D1HwndRenderTarget* rt_ = nullptr; // render target

		Font* font_;
		ID2D1SolidColorBrush* fg_; // foreground (text) style
		ID2D1SolidColorBrush* bg_; // background style

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
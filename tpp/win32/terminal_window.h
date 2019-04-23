#pragma once
#ifdef WIN32

#include <unordered_map>

#include <windows.h>
#include <d2d1.h>
#include <dwrite.h>

#include "helpers/log.h"

#include "../base_terminal_window.h"

namespace tpp {

	class Application;

	template<>
	inline FontSpec<HFONT> * FontSpec<HFONT>::Create(vterm::Font font, unsigned height) {
		HFONT handle = CreateFont(
			height * font.size(),
			0, // default escapment
			0, // default orientation 
			0, // default width, we must calculate the width when done
			font.bold() ? FW_BOLD : FW_DONTCARE,
			font.italics(),
			font.underline(),
			font.strikeout(),
			DEFAULT_CHARSET,
			OUT_OUTLINE_PRECIS,
			CLIP_DEFAULT_PRECIS,
			CLEARTYPE_QUALITY,
			FIXED_PITCH,
			"Iosevka NF");
		// we have the handle for the font, now determine the width of it
		ABC abc;
		HDC dc = CreateCompatibleDC(nullptr);
		SelectObject(dc, handle);
		GetCharABCWidths(dc, 'm', 'm', &abc);
		DeleteObject(dc);
		// TODO this feels wrong, I should be able to determine this precisely w/o + 1 magic
		unsigned width = abc.abcA + abc.abcB + abc.abcC;
		return new FontSpec<HFONT>(font, width, height, handle);
	}

	class TerminalWindow : public BaseTerminalWindow {
	public:
		typedef FontSpec<HFONT> Font;

		TerminalWindow(Application * app, TerminalSettings * settings);

		void show() override {
			ShowWindow(hWnd_, SW_SHOWNORMAL);
		}

		void hide() override {
			NOT_IMPLEMENTED;
		}

		void redraw() override {
			if (buffer_ != nullptr) {
				DeleteObject(buffer_);
				buffer_ = nullptr;
			}
			InvalidateRect(hWnd_, /* rect */ nullptr, /* erase */ false);
		}

	protected:

		/** Just deleting a terminal window is not allowed, therefore protected.
		 */
		~TerminalWindow() override;

		void repaint(vterm::Terminal::RepaintEvent & e) override;

		void doSetFullscreen(bool value) override;

		void doTitleChange(vterm::VT100::TitleEvent & e) override;

		/** Deletes the double buffer object. 
		 */
		void doInvalidate() override {
			DeleteObject(buffer_);
			buffer_ = nullptr;
			InvalidateRect(hWnd_, /* rect */ nullptr, /* erase */ false);
		}

		void doPaint() override;

		void doSetForeground(vterm::Color const& fg) override {
			SetTextColor(bufferDC_, RGB(fg.red, fg.green, fg.blue));
		}

		void doSetBackground(vterm::Color const& bg) override {
			SetBkColor(bufferDC_, RGB(bg.red, bg.green, bg.blue));
		}

		void doSetFont(vterm::Font font) override {
			SelectObject(bufferDC_, Font::GetOrCreate(font, settings_->defaultFontHeight, zoom_)->handle());
		}

		void doDrawCell(unsigned col, unsigned row, vterm::Cell const& c) override {
			wchar_t wc = c.c.toWChar();
			TextOutW(bufferDC_, col * cellWidthPx_, row * cellHeightPx_, &wc, 1);
		}

		void doDrawCursor(unsigned col, unsigned row, vterm::Cell const& c) override {
			doSetForeground(c.fg);
			doSetFont(c.font);
			SetBkMode(bufferDC_, TRANSPARENT);
			doDrawCell(col, row, c);
		}

	private:

		static constexpr WPARAM MSG_TITLE_CHANGE = 0;

		friend class Application;

		//void updateBuffer();

		/** Maps win32 virtual keys to their vterm equivalents. 
		 */
		static vterm::Key GetKey(WPARAM vk);

		static LRESULT CALLBACK EventHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

		HWND hWnd_;

		/** Contains the shadow buffer for the window. The bitmap itself and the memory only buffer device context is required. */
		HBITMAP buffer_;
		HDC bufferDC_;

		/** Placement of the window to which the window is restored after fullscreen toggle. 
		 */
		WINDOWPLACEMENT wndPlacement_;

		/** Width and height of the window frame so that the width and height of the window can be adjusted accordingly. 
		 */
		unsigned frameWidth_;
		unsigned frameHeight_;

		static std::unordered_map<HWND, TerminalWindow *> Windows_;

		static constexpr UINT_PTR TIMERID_BLINK = 34;


	};

} // namespace tpp
#endif
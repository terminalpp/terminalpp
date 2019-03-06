#pragma once

#include <unordered_map>

#include <windows.h>

#include "vterm/virtual_terminal.h"

#include "../settings.h"
#include "../terminal_window.h"

namespace tpp {

	/** Font description for the Win32 GDI rendering. 

	    Contains the handle to the specified font as well as associated properties and the vterm::Font for which the GDI font was created. 
	 */
	class GDIFont : public Font {
	public:

		/** Height of the base font. 

		    TODO this is to be refactored to some settings or similar stuff. 
		 */
		static constexpr unsigned FontHeight = 16;

		/** Name of the base font. 

		    TODO this is to be refactored to some settings or so. 
		 */
		static constexpr char const * FontFamily = "Iosevka NF";

		/** Returns font corresponding to the given vterm::Font. If such font has not been requested yet, new one is created and cached. 
		 */
		static GDIFont * GetOrCreate(HDC hdc, vterm::Font const & font) {
			// disable blinking in the font since blinking is not property of font
			auto i = Fonts_.find(font);
			if (i != Fonts_.end()) 
				return i->second;
			HFONT handle = CreateFont(
				FontHeight * font.size(),
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
				FontFamily);
			ASSERT(handle != nullptr) << "Font not created";
			// we have the handle for the font, now determine the width of it
			ABC abc;
			SelectObject(hdc, handle);
			GetCharABCWidths(hdc, 'm', 'm', &abc);
			unsigned width = abc.abcA + abc.abcB + abc.abcC;
			GDIFont * f = new GDIFont(font, handle, width, FontHeight * font.size());
			Fonts_.insert(std::make_pair(font, f));
			return f;
		}

		HFONT handle() const {
			return h_;
		}

	protected:

		GDIFont(vterm::Font const & font, HFONT handle, unsigned width, unsigned height) :
			Font(font, width, height),
			h_(handle) {
		}

		HFONT h_;

		/** Cache of already created fonts so that we do not have to recreate them for each window, but keep them app-wide. 

		    TODO perhaps limit the number of fonts that can be cached at any given time? Or not necessary because the # of font options is limited. 

			TODO perhaps this should go in the gui_renderer, depending on how fonts are dealt with in other renderer backends. 
		 */
		static std::unordered_map<vterm::Font, GDIFont *> Fonts_;
	};

	/** Terminal renderer with Win32 GDI as its backend. 
	 */
	class GDITerminalWindow : public TerminalWindow {
	public:

		~GDITerminalWindow() {
			DeleteObject(buffer_);
			DeleteObject(memoryBuffer_);
		}

		/** Makes the window visible. 
		 */
		void show() override {
			ShowWindow(hWnd_, SW_SHOWNORMAL);
		}

	protected:
		friend class GDIApplication;

		GDITerminalWindow(HWND hWnd) :
			hWnd_(hWnd),
			buffer_(nullptr),
		    memoryBuffer_(CreateCompatibleDC(nullptr)) {
			Windows_.insert(std::make_pair(hWnd, this));
		}

		void resize(unsigned width, unsigned height) override; 

		/** Redraws the appropriate rectangle of the terminal.
		 */
		virtual void repaint(unsigned left, unsigned top, unsigned cols, unsigned rows);

		/** Paints the shadow buffer with the contents of the terminal. 
		 */
		void paintShadowBuffer(unsigned left, unsigned top, unsigned cols, unsigned rows);

	private:

		/** Handles windows GDI events.
		 */
		static LRESULT CALLBACK EventHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

		/** Actually paints the window. Dispatched from the EventHandler. 
		 */
		void doPaint();

		/** Handle to the window object. */
		HWND hWnd_;

		/** Contains the shadow buffer for the window. */
		HBITMAP buffer_;

		/** The memory buffer's device context. */
		HDC memoryBuffer_;

		/** The width and height of the window border, which is required to adjust the window size to preserve the desired renderer's client area width and height.

		    For now these are kept globally since all windows have the same window class.
		 */
		static unsigned BorderWidth_;
		static unsigned BorderHeight_;

		/** List of all terminal windows.

			TODO perhaps this should better go to the application - I haven't yet decided on the multi-window usage.
		 */
		static std::unordered_map<HWND, GDITerminalWindow *> Windows_;


	}; // GDITerminalWindow

} // namespace tpp
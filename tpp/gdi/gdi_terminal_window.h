#pragma once

#include <unordered_map>

#include <windows.h>


#include "vterm/virtual_terminal.h"

#include "../settings.h"
#include "../terminal_window.h"
#include "gdi_application.h"

namespace tpp {

	/** The terminal window. 

	    Windows are created by the Application instance. Each terminal window is attached to a screen buffer and displays its contents.  
	 */
	class GDITerminalWindow : public TerminalWindow {
	public:

		/** Shows the terminal window. 
		 */
		void show() override {
			ShowWindow(hWnd_, SW_SHOWNORMAL);
		}

		void hide() override {
			NOT_IMPLEMENTED;
		}

		/** Destroys the terminal window. 
		 */
		~GDITerminalWindow() override {
			DeleteObject(buffer_);
			DeleteDC(memoryBuffer_);
		}

	protected:
		/** Repaints the specified rectangle of the terminal window and then posts the WM_PAINT message to the window.
		 */
		void repaintTerminal(vterm::RepaintEvent & e) override;

		void resize(unsigned width, unsigned height) override;

	private:
		friend class GDIApplication;

		/** Handles windows GDI events.
		 */
		static LRESULT CALLBACK EventHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

		/** Creates the terminal window.
		 */
		GDITerminalWindow(HWND hWnd);

		/** Gets the size of the window from GDI and updates the size of the window and attached terminal accordingly. 
		 */
		void updateWindowSize();

		HFONT getFont(vterm::Font const & font);

		unsigned calculateFontWidth(HDC hdc, HFONT font);

		/** Sets the width_ and height_ properties to the current width and height of the window. 
		 */
		void getWindowSize();

		/** Handle to GDI window object. */
		HWND hWnd_;

		/** Contains the shadow buffer for the window. */
		HBITMAP buffer_;

		/** The memory buffer's device context. */
		HDC memoryBuffer_;

		/** Available fonts. 
		 */
		std::unordered_map<vterm::Font, HFONT, vterm::Font::BlinkIgnoringHash,	 vterm::Font::BlinkIgnoringComparator> fonts_;

		/** List of all terminal windows. 

		    TODO perhaps this should better go to the application - I haven't yet decided on the multi-window usage. 
		 */
		static std::unordered_map<HWND, GDITerminalWindow *> Windows_;
	};



} // namespace tpp
#pragma once

#include <unordered_map>

#include <windows.h>


#include "vterm/screen_buffer.h"
#include "vterm/char.h"

#include "settings.h"
#include "application.h"

namespace tpp {

	/** The terminal window. 

	    Windows are created by the Application instance. Each terminal window is attached to a screen buffer and displays its contents.  
	 */
	class TerminalWindow {
	public:

		/** Shows the terminal window. 
		 */
		void show() {
			ShowWindow(hwnd_, SW_SHOWNORMAL);
		}

		/** Returns the screen buffer associated with the window.

		    If the terminal window is not associated with a screen buffer, returns nullptr. 
		 */
		vterm::ScreenBuffer const * screenBuffer() const {
			return screenBuffer_;
		}
		
		vterm::ScreenBuffer * screenBuffer() {
			return screenBuffer_;
		}

	private:
		friend class Application;

		/** Creates the terminal window. 
		 */
		TerminalWindow(HWND hwnd, vterm::ScreenBuffer * screenBuffer);

		/** Handles windows GDI events. 
		 */
		static LRESULT CALLBACK EventHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

		HFONT getFont(vterm::Font const & font);

		unsigned calculateFontWidth(HDC hdc, HFONT font);

		/** Paints the window of the terminal. 

		    Goes through all the cells 
		 */
		void doPaint();

		/** Handle to GDI window object. 
		 */
		HWND hwnd_;

		/** Associated screen buffer. 
		 */
		vterm::ScreenBuffer * screenBuffer_;

		/** Available fonts. 
		 */
		std::unordered_map<vterm::Font, HFONT, vterm::Font::BlinkIgnoringHash,	 vterm::Font::BlinkIgnoringComparator> fonts_;

		/** Width of the font. 
		 */
		unsigned fontWidth_;

		/** List of all terminal windows. 

		    TODO perhaps this should better go to the application - I haven't yet decided on the multi-window usage. 
		 */
		static std::unordered_map<HWND, TerminalWindow *> Windows_;
	};



} // namespace tpp
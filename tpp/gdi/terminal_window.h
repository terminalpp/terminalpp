#pragma once

#include <unordered_map>

#include <windows.h>


#include "vterm/virtual_terminal.h"

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
			ShowWindow(hWnd_, SW_SHOWNORMAL);
		}

		/** Returns the screen buffer associated with the window.

		    If the terminal window is not associated with a screen buffer, returns nullptr. 
		 */
		Terminal const * terminal() const {
			return terminal_;
		}
		
		Terminal * terminal() {
			return terminal_;
		}

		size_t width() const {
			return width_;
		}

		size_t height() const {
			return height_;
		}

		/** Destroys the terminal window. 
		 */
		~TerminalWindow() {
			DeleteObject(buffer_);
			DeleteDC(memoryBuffer_);
			// TODO also remove from the list of windows.
		}

	private:
		friend class Application;

		/** Creates the terminal window. 
		 */
		TerminalWindow(HWND hwnd, Terminal * terminal);

		/** Handles windows GDI events. 
		 */
		static LRESULT CALLBACK EventHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

		HFONT getFont(vterm::Font const & font);

		unsigned calculateFontWidth(HDC hdc, HFONT font);

		/** Paints the window of the terminal. 

		    Goes through all the cells 
		 */
		void doPaint();

		void refresh(unsigned left, unsigned top, unsigned cols, unsigned rows);

		/** Sets the width_ and height_ properties to the current width and height of the window. Returns true if there was any change from the stored values, false otherwise. 

		    If there is a change, updates the size of the attached terminal, but does not raise the onResize event. Also updates the bitmap to reflect the new size. 
		*/
		bool getWindowSize();

		/** Handle to GDI window object. 
		 */
		HWND hWnd_;

		/** Contains the shadow buffer for the window. 
		 */
		HBITMAP buffer_;

		/** The memory buffer's device context. */
		HDC memoryBuffer_;

		/** Associated virtual terminal. 
		 */
		Terminal * terminal_;

		/** Width of the terminal window. 
		 */
		size_t width_;

		/** Height of the terminal window. 
		 */
		size_t height_;

		/** Available fonts. 
		 */
		std::unordered_map<vterm::Font, HFONT, vterm::Font::BlinkIgnoringHash,	 vterm::Font::BlinkIgnoringComparator> fonts_;

		/** Width of the font. 
		 */
		unsigned fontWidth_;

		/** Height of the font. 
		 */
		unsigned fontHeight_;

		/** List of all terminal windows. 

		    TODO perhaps this should better go to the application - I haven't yet decided on the multi-window usage. 
		 */
		static std::unordered_map<HWND, TerminalWindow *> Windows_;
	};



} // namespace tpp
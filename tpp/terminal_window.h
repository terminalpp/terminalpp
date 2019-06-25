#pragma once

#include <cmath>

#include "helpers/time.h"
#include "helpers/log.h"

#include "vterm/vt100.h"


#include "config.h"

namespace tpp {

	class Application;
	class Session;

	/** Single terminal window.

	    The terminal window is a vterm renderer that can display the contents of the associated terminal. This class provides the common, platform independent functionality. 
	 */
	class TerminalWindow : public vterm::Terminal::Renderer {
	public:

		/** Encapsulates the properties of the window so that they can be obtained and updated easily. 
		 */
		class Properties {
		public:
			unsigned cols;
			unsigned rows;
			unsigned fontSize;
			double zoom;

			/** Creates the properties object and fills in the values. 
			 */
			Properties(unsigned cols = DEFAULT_TERMINAL_COLS, unsigned rows = DEFAULT_TERMINAL_ROWS, unsigned fontSize = DEFAULT_TERMINAL_FONT_SIZE, double zoom = 1) :
				cols(cols),
				rows(rows),
				fontSize(fontSize),
				zoom(zoom) {
			}

			/** Creates the properties object and fills its values from given terminal window. 
			 */
			Properties(TerminalWindow const* tw);
		}; // TerminalWindow::Properties


		/** Returns the session the window belongs to.
		 */
		Session* session() const {
			return session_;
		}

		/** Returns the title of the window. 
		 */
		std::string const& title() {
			return title_;
		}

		/** Returns the zoom level of the window. 
		 */
		double zoom() const {
			return zoom_;
		}

		/** Sets the zoom level of the window.

			Zoom value of 1.0 means default size.
		 */
		void setZoom(double value) {
			if (value != zoom_) {
				zoom_ = value;
				doSetZoom(value);
			}
		}

		bool fullscreen() const {
			return fullscreen_;
		}

		void setFullscreen(bool value = true) {
			if (value != fullscreen_) {
				fullscreen_ = value;
				doSetFullscreen(value);
			}
		}


		// methods --------------------------------------------------------------------------------------

		virtual void show() = 0;
		virtual void hide() = 0;
		virtual void close() = 0;

		/** TODO can this be simplified and not dealt with in the TerminalWindow? 
		 */
		virtual void inputReady() = 0;

		/** Redraws the window completely from the attached vterm. 
		 */
		void redraw() {
			doInvalidate(true);
		}

	protected:

		// TODO how much of the protected stuff should actually be private? 

		/** Because the blink attribute has really nothing to do with the font itself, this simple functions strips its value from given font so that fonts excluding the blinking can be easily compared. */
		static vterm::Font DropBlink(vterm::Font font) {
			font.setBlink(false);
			return font;
		}
		

		TerminalWindow(Session* session, Properties const& properties, std::string const& title);

		void paint() {
			helpers::Timer t;
			t.start();
			unsigned cells = doPaint();
			double time = t.stop();
			//LOG << "Repaint event: cells: " << cells << ",  ms: " << (time * 1000);
		}

		/** Returns the selected area. 
		 */
		vterm::Selection selectedArea() const {
			if (selectionStart_.row < selectionEnd_.row) {
				if (selectionStart_.row + 1 == selectionEnd_.row && selectionStart_.col > selectionEnd_.col)
					return vterm::Selection(selectionEnd_.col, selectionStart_.row, selectionStart_.col, selectionEnd_.row);
				else
				    return vterm::Selection(selectionStart_, selectionEnd_);
			} else {
				return vterm::Selection(selectionEnd_.col, selectionEnd_.row - 1, selectionStart_.col, selectionStart_.row + 1);
			}
		}

		void clearSelection() {
			selectionStart_.col = 0;
			selectionStart_.row = 0;
			selectionEnd_.col = 0;
			selectionEnd_.row = 0;
			doInvalidate(false);
		}

		/** Called when appropriate events are received by the windows' event loop.

			Since multiple events of same type may be received, we first check that the value ideed differs. If it does the focusChanged virtual method is called with the new value.
		 */
		void focusChangeMessageReceived(bool focus) {
			if (focused_ != focus) {
				focused_ = focus;
				focusChanged(focused_);
			}
		}

        void repaint(vterm::Terminal::RepaintEvent & e) override {
            doInvalidate(e->invalidateAll);
        }

        void titleChange(vterm::Terminal::TitleChangeEvent & e) override {
            title_ = *e;
        }

		virtual void blinkTimer() {
			blink_ = ! blink_;
			doInvalidate(false);
		}

		/** Called when the window's focus changes. 

		    The new value of the focus is the argument. 
		 */
		virtual void focusChanged(bool focused) {
			LOG << "Focus changed: " << focused;
		}

		/** Handles resize of the window's client area (in pixels). 

		    Recalculates the number of columns and rows displayabe and calls the renderer's resize method which in turn updates the underlying terminal. When the terminal changes, it would trigger the repaint event on the window. 
		 */
		virtual void windowResized(unsigned widthPx, unsigned heightPx) {
			widthPx_ = widthPx;
			heightPx_ = heightPx;
			resize(widthPx / cellWidthPx_, heightPx / cellHeightPx_);
		}

		/** Sets zoom level for the window. 

		    Updates the cellWidthPx and cellHeightPx values based on the desired zoom level. To make sure that the cells are rendered correctly, the cell height is updated via the zoom level, but the cell width is calculated from the font size of the given font.
		 */
		virtual void doSetZoom(double value);

		virtual void doSetFullscreen(bool value) = 0;

        /** Sends given character to the attached terminal. 
         */  
        virtual void keyChar(vterm::Char::UTF8 c);

        /** Handles the key press event. 
         */ 
        virtual void keyUp(vterm::Key key);

        /** Handles the key release event.
         */
        virtual void keyDown(vterm::Key key);


		void convertMouseCoordsToCells(unsigned & x, unsigned & y) {
			x = x / cellWidthPx_;
			y = y / cellHeightPx_;
		}

		virtual void mouseMove(unsigned x, unsigned y);
		virtual void mouseDown(unsigned x, unsigned y, vterm::MouseButton button);
		virtual void mouseUp(unsigned x, unsigned y, vterm::MouseButton button);
		virtual void mouseWheel(unsigned x, unsigned y, int offset);

		/** Invalidates the contents of the window and triggers a repaint.

            The base window sets the invalidation flag and the implementations should provide the repaint trigger. 
		 */
		virtual void doInvalidate(bool forceRepaint) {
			forceRepaint_ = forceRepaint;
        }

		virtual void clipboardPaste() = 0;

		/** Paints the window.
		 */
		virtual unsigned doPaint() = 0;

		/** Sets the foreground color for next cells or cursor to be drawn.  
		 */
		virtual void doSetForeground(vterm::Color const& fg) = 0;

		/** Sets the background color for next cells to be drawn note that backrgound color should not be used when drawing the cursor. 
		 */
		virtual void doSetBackground(vterm::Color const& bg) = 0;

		/** Sets the font for next cells or cursor to be drawn.
		 */
		virtual void doSetFont(vterm::Font font) = 0;

		/** Draws single cell. 
		 */
		virtual void doDrawCell(unsigned col, unsigned row, vterm::Terminal::Cell const& c) = 0;

		/** Draws the cursor, described as a cell. 

		    Only the font, character and foreground color from the cell should be used. 
		 */
		virtual void doDrawCursor(unsigned col, unsigned row, vterm::Terminal::Cell const& c) = 0;

		/** Clears the entire window. 
		 */
		virtual void doClearWindow() = 0;

		/** Updates the terminal buffer displayed. 

		    Triggers repaint of all dirty terminal cells (or all cells if forceDirty is true) and the cursor. 
		 */
		unsigned drawBuffer();

		/** Session the window belongs to. 
		 */
		Session* session_;

		/** Width and height of the window client area in pixels. 
		 */
		unsigned widthPx_;
		unsigned heightPx_;

		/** True if the window is focused, false otherwise. 
		 */
		bool focused_;

		/** Title of the terminal window. 
		 */
		std::string title_;

		/** Zoom level of the window. 
		 */
		double zoom_;

		/** If true, the paint method should clear the entire window first. 

		    Such as when zoom is changed and there can be extra space around the visible columns. 
		 */
		bool clearWindow_;

		/** Determines whether the window is fullscreen or not. 
		 */
		bool fullscreen_;

		/** Font size at zoom level 1
		 */
		unsigned fontSize_;

		/** Width of a single cell in pixels. 
		 */
		unsigned cellWidthPx_;

		/** Height of a single cell in pixels. 
		 */
		unsigned cellHeightPx_;

		/** Toggle for the visibility of the blinking text and cursor. 

		    Should be toggled by the terminal window implementation in regular intervals. 
		 */
	    bool blink_;

		/** If true, the entire window contents has been invalidated and the window should be repainted.

			If the window contents is buffered, the flag also means that the buffer must be recreated (such as when window size changes).
		 */
		bool forceRepaint_;

		/** Last known mouse coordinates in terminal columns & rows (not in pixels).
		 */
		unsigned mouseCol_;
		unsigned mouseRow_;

		/** Mouse selected region of the terminal, if any. 
		 */
		helpers::Point selectionStart_;
		helpers::Point selectionEnd_;
		bool selecting_;

	};


} // namespace tpp

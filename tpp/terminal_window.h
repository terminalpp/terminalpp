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
			Properties(unsigned cols, unsigned rows, unsigned fontSize, double zoom = 1) :
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

	protected:

		/** Because the blink attribute has really nothing to do with the font itself, this simple functions strips its value from given font so that fonts excluding the blinking can be easily compared. */
		static vterm::Font DropBlink(vterm::Font font) {
			font.setBlink(false);
			return font;
		}

		TerminalWindow(Session* session, Properties const& properties, std::string const& title);

		void paint() {
			doPaint();
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
                // check if the selection is actually empty
                if (selectionStart_ == selectionEnd_ && selectionStart_.col == 0 && selectionEnd_.row == 0)
                    return vterm::Selection(selectionStart_, selectionEnd_);
				return vterm::Selection(selectionEnd_.col, selectionEnd_.row - 1, selectionStart_.col, selectionStart_.row + 1);
			}
		}

        /** Called when the selection should be cleared. 

            Setting the manual flag to false indicates that the selection is to be cleared due to other things than direct user interaction with the window. 
         */
		virtual void selectionClear(bool manual = true) {
            MARK_AS_UNUSED(manual);
			selectionStart_.col = 0;
			selectionStart_.row = 0;
			selectionEnd_.col = 0;
			selectionEnd_.row = 0;
		}

        virtual void selectionSet() {
            // do nothing, but children can 
        }

        /** Selection paste
         
            Pastes selection without going through the clipboard. This is the implementation of the X11 PRIMARY selection. Returns true if the request was serviced. The implementation should override the function and first call the parent to see if the selection can be obtained simply from the current window. If the call returns true it should then use whatever means available to determine the primary selection on the given architecture. 
         */
        virtual bool selectionPaste();

		virtual void clipboardPaste() = 0;


		/** Called when appropriate events are received by the windows' event loop.

			Since multiple events of same type may be received, we first check that the value ideed differs. If it does the focusChanged virtual method is called with the new value.
		 */
		void focusChangeMessageReceived(bool focus) {
			if (focused_ != focus) {
				focusChanged(focus);
			}
		}

        void repaint(vterm::Terminal::RepaintEvent & e) override {
			MARK_AS_UNUSED(e);
			dirty_ = true;
        }

        void titleChange(vterm::Terminal::TitleChangeEvent & e) override {
            title_ = *e;
        }

		virtual void fpsTimer() {
			if (--blinkCounter_ == 0) {
				blinkCounter_ = (*config::FPS) / 2;
				blink_ = !blink_;
				blinkDirty_ = true;
				doInvalidate();
			} else if (dirty_) {
				doInvalidate();
			}
		}

		/** Called when the window's focus changes. 

		    The new value of the focus is the argument. 
		 */
		virtual void focusChanged(bool focused) {
			focused_ = focused;
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
        virtual void keyChar(helpers::Char c);

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
		virtual void doInvalidate() {
			dirty_ = true;
        }

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

		/** Updates the terminal buffer displayed. 

		    Triggers repaint of all dirty terminal cells (or all cells if forceDirty is true) and the cursor. 
		 */
		unsigned drawBuffer(bool forceDirty);

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

		/** True if blinking state changed, i.e. all cells with blinking text should be considered dirty. 
		 */
		bool blinkDirty_;

		unsigned blinkCounter_;

		bool dirty_;

		/** Last known mouse coordinates in terminal columns & rows (not in pixels).
		 */
		unsigned mouseCol_;
		unsigned mouseRow_;

		/** Mouse selected region of the terminal, if any. 
		 */
		vterm::Point selectionStart_;
		vterm::Point selectionEnd_;
		bool selecting_;
	};


} // namespace tpp

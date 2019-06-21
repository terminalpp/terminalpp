#pragma once

#include <cmath>

#include "helpers/time.h"
#include "helpers/log.h"

#include "vterm/vt100.h"


#include "config.h"

namespace tpp {

	class Application;
	class Session;

	/** Stores and retrieves font objects so that they do not have to be created each time they are needed. 

	    Templated by the actual font handle, which is platform dependent. 

	 */
	template<typename T>
	class FontSpec {
	public:

		/** Return a font for given terminal font description and zoom. 
		 */
		static FontSpec * GetOrCreate(vterm::Font const & font, unsigned height) {
			vterm::Font f = StripEffects(font);
			unsigned id = (height << 8) + f.raw();
			auto i = Fonts_.find(id);
			if (i == Fonts_.end())
				i = Fonts_.insert(std::make_pair(id, Create(font, height))).first;
			return i->second;
		}

		vterm::Font font() const {
			return font_;
		}

		T const & handle() const {
			return handle_;
		}

		unsigned widthPx() const {
			return widthPx_;
		}

		unsigned heightPx() const {
			return heightPx_;
		}

		/** Does nothing, but explicitly mentioned so that it can be specialized when necessary. 
		 */
		~FontSpec() {
		}

	private:

		/** Strips effects that does not alter the font selection on the given platform. 

		    By default strips only the blinking attribute, implementations can override this to strip other font effects as well. 
		 */
		static vterm::Font StripEffects(vterm::Font const & font) {
			vterm::Font result(font);
			result.setBlink(false);
			return result;
		}

		FontSpec(vterm::Font font, unsigned width, unsigned height, T const & handle) :
			font_(font),
			widthPx_(width),
			heightPx_(height),
			handle_(handle) {
		}

		vterm::Font font_;
		unsigned widthPx_;
		unsigned heightPx_;
		T handle_;

		/** This must be implemented in platform specific code. 
		 */
		static FontSpec * Create(vterm::Font font, unsigned baseHeight);

		static std::unordered_map<unsigned, FontSpec *> Fonts_;
	};


	template<typename T>
	std::unordered_map<unsigned, FontSpec<T> *> FontSpec<T>::Fonts_;


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
			unsigned fontWidth;
			unsigned fontHeight;
			double zoom;

			/** Creates the properties object and fills in the values. 
			 */
			Properties(unsigned cols = DEFAULT_TERMINAL_COLS, unsigned rows = DEFAULT_TERMINAL_ROWS, unsigned fontWidth = 0, unsigned fontHeight = DEFAULT_TERMINAL_FONT_HEIGHT, double zoom = 1) :
				cols(cols),
				rows(rows),
				fontWidth(fontWidth),
				fontHeight(fontHeight),
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

		/** Because the blink attribute has really nothing to do with the font itself, this simple functions strips its value from given font so that fonts excluding the blinking can be easily compared. */
		static vterm::Font DropBlink(vterm::Font font) {
			font.setBlink(false);
			return font;
		}
		
		TerminalWindow(Session * session, Properties const & properties, std::string const & title) :
			vterm::Terminal::Renderer(properties.cols, properties.rows),
			session_(session),
			widthPx_(properties.fontWidth * properties.cols),
			heightPx_(properties.fontHeight * properties.rows),
			title_(title),
			zoom_(properties.zoom),
			fullscreen_(false),
			fontWidth_(properties.fontWidth),
			fontHeight_(properties.fontHeight),
			cellWidthPx_(static_cast<unsigned>(properties.fontWidth * properties.zoom)),
			cellHeightPx_(static_cast<unsigned>(properties.fontHeight * properties.zoom)),
		    blink_(true),
		    mouseCol_(0),
		    mouseRow_(0) {
		}

		void paint() {
			helpers::Timer t;
			t.start();
			unsigned cells = doPaint();
			double time = t.stop();
			LOG << "Repaint event: cells: " << cells << ",  ms: " << (time * 1000);
		}

        void repaint(vterm::Terminal::RepaintEvent & e) override {
            doInvalidate(e->invalidateAll);
        }

        void titleChange(vterm::Terminal::TitleChangeEvent & e) override {
            title_ = *e;
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

		    Updates the cellWidthPx and cellHeightPx values based on the desired zoom level. 
		 */
		virtual void doSetZoom(double value) {
			clearWindow_ = true;
			// update width & height of the cell
			cellWidthPx_ = static_cast<unsigned>(std::round(value * fontWidth_));
			cellHeightPx_ = static_cast<unsigned>(std::round(value * fontHeight_));
			// resize the terminal properly
			resize(widthPx_ / cellWidthPx_, heightPx_ / cellHeightPx_);
		}

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
		virtual void clipboardCopy(std::string const& str) = 0;

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
		unsigned fontWidth_;
		unsigned fontHeight_;

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

	};


} // namespace tpp
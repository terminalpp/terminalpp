#pragma once

#include <cmath>

#include "vterm/renderer.h"
#include "vterm/vt100.h"

namespace tpp {

	/** Stores and retrieves font objects so that they do not have to be created each time they are needed. 

	    Templated by the actual font handle, which is platform dependent. 
	 */
	template<typename T>
	class FontSpec {
	public:

		/** Return a font for given terminal font description and zoom. 
		 */
		static FontSpec * GetOrCreate(vterm::Font font, unsigned fontHeight, double zoom) {
			// disable blinking as it is not really important for font selection
			font.setBlink(false);
			unsigned height = static_cast<unsigned>(std::round(zoom * fontHeight));
			unsigned id = (height << 8) + font.raw();
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

	private:

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
		static FontSpec * Create(vterm::Font font, unsigned height);

		static std::unordered_map<unsigned, FontSpec *> Fonts_;
	};


	template<typename T>
	std::unordered_map<unsigned, FontSpec<T> *> FontSpec<T>::Fonts_;


	/** Description of settings relevant for terminal windows. 

	 */
	class TerminalSettings {
	public:

		/** Basic title for the terminal window. 
		 */
		std::string defaultTitle = "terminal++";

		/** Default width and height of the terminal display (in terminal rows and cols, not in pixels). 
		 */
		unsigned defaultCols = 80;
		unsigned defaultRows = 25;

		/** Default width and height in pixels, of the selected font. I.e. the width and height of a single terminal cell. */
		unsigned defaultFontHeight = 16;
		unsigned defaultFontWidth = 0;

		/** Default zoom of the window. 
		 */
		double defaultZoom = 1.0;

		/** Determines whether the window starts in fullscreen mode or not. 
		 */
		double fullscreen = false;

	}; // tpp::TerminalSettings


	/** Single terminal window.

	    The terminal window is a vterm renderer that can display the contents of the associated terminal. This class provides the common, platform independent functionality. 
	 */
	class BaseTerminalWindow : public vterm::Renderer {
	public:

		TerminalSettings * settings() const {
			return settings_;
		}

		/** Returns the title of the window. 
		 */
		std::string const & title() const {
			return title_;
		}

		/** Returns the zoom level of the window. 
		 */
		double zoom() const {
			return zoom_;
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

		/** Sets the zoom level of the window. 

		    Zoom value of 1.0 means default size.
		 */
		void setZoom(double value) {
			if (value != zoom_) {
				zoom_ = value;
				doSetZoom(value);
			}
		}

		// methods --------------------------------------------------------------------------------------

		virtual void show() = 0;
		virtual void hide() = 0;

		/** Redraws the window completely from the attached vterm. 
		 */
		virtual void redraw() {
			doInvalidate();
			//doPaint();
		}

	protected:

		/** Because the blink attribute has really nothing to do with the font itself, this simple functions strips its value from given font so that fonts excluding the blinking can be easily compared. */
		static vterm::Font DropBlink(vterm::Font font) {
			font.setBlink(false);
			return font;
		}
		
		BaseTerminalWindow(TerminalSettings * settings) :
			vterm::Renderer(settings->defaultCols, settings->defaultRows),
			settings_(settings),
			title_(settings->defaultTitle),
			widthPx_(settings->defaultFontWidth * settings->defaultCols),
			heightPx_(settings->defaultFontHeight * settings->defaultRows),
			zoom_(settings->defaultZoom),
			fullscreen_(settings->fullscreen),
			cellWidthPx_(settings->defaultFontWidth * settings->defaultZoom),
			cellHeightPx_(settings->defaultFontHeight * settings->defaultZoom),
		    blink_(false) {

		}

		void doAttachTerminal(vterm::Terminal * terminal) override {
			Renderer::doAttachTerminal(terminal);
			vterm::VT100 * vt = dynamic_cast<vterm::VT100 *>(terminal);
			if (vt != nullptr) {
				vt->onTitleChange += HANDLER(BaseTerminalWindow::doTitleChange);
			}
		}

		void doDetachTerminal(vterm::Terminal * terminal) override {
			vterm::VT100 * vt = dynamic_cast<vterm::VT100 *>(terminal);
			if (vt != nullptr) {
				vt->onTitleChange -= HANDLER(BaseTerminalWindow::doTitleChange);
			}
			Renderer::doDetachTerminal(terminal);
		}


		/** Handles resize of the window's client area (in pixels). 

		    Recalculates the number of columns and rows displayabe and calls the renderer's resize method which in turn updates the underlying terminal. When the terminal changes, it would trigger the repaint event on the window. 
		 */
		virtual void resizeWindow(unsigned widthPx, unsigned heightPx) {
			doInvalidate();
			widthPx_ = widthPx;
			heightPx_ = heightPx;
			resize(widthPx / cellWidthPx_, heightPx / cellHeightPx_);
		}

		/** Sets zoom level for the window. 

		    Updates the cellWidthPx and cellHeightPx values based on the desired zoom level. 
		 */
		virtual void doSetZoom(double value) {
			// update width & height of the cell
			cellWidthPx_ = static_cast<unsigned>(std::round(value * settings_->defaultFontWidth));
			cellHeightPx_ = static_cast<unsigned>(std::round(value * settings_->defaultFontHeight));
			NOT_IMPLEMENTED;
			// resize the terminal properly
		}

		virtual void doSetFullscreen(bool value) = 0;

		virtual void doTitleChange(vterm::VT100::TitleEvent & e) = 0;

		/** Invalidates the contents of the window without triggering immediate repaint. 
		 */
		virtual void doInvalidate() = 0;

		/** Paints the window. 
		 */
		virtual void doPaint() = 0;

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
		virtual void doDrawCell(unsigned col, unsigned row, vterm::Cell const& c) = 0;

		/** Draws the cursor, described as a cell. 

		    Only the font, character and foreground color from the cell should be used. 
		 */
		virtual void doDrawCursor(unsigned col, unsigned row, vterm::Cell const& c) = 0;

		/** Updates the terminal buffer displayed. 

		    Triggers repaint of all dirty terminal cells (or all cells if forceDirty is true) and the cursor. 
		 */
		void doUpdateBuffer(bool forceDirty = false);

		TerminalSettings * settings_;

		std::string title_;

		/** Width and height of the window client area in pixels. 
		 */
		unsigned widthPx_;
		unsigned heightPx_;

		/** Zoom level of the window. 
		 */
		double zoom_;

		/** Determines whether the window is fullscreen or not. 
		 */
		bool fullscreen_;

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

	};


} // namespace tpp
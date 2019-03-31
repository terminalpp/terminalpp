#pragma once

#include <cmath>

#include "vterm/renderer.h"
#include "vterm/vt100.h"

namespace tpp {

	/** Stores and retrieves font objects so that they do not have to be created each time they are needed. 
	 */
	template<typename T>
	class Font {
	public:

		/** Return a font for given terminal font description and zoom. 
		 */
		static Font * GetOrCreate(vterm::Font font, unsigned fontHeight, double zoom) {
			// disable blinking as it is not really important for font selection
			font.setBlink(false);
			unsigned height = static_cast<unsigned>(std::round(zoom * fontHeight));
			unsigned id = (height << 8) + font.raw();
			auto i = Fonts_.find(id);
			if (i == Fonts_.end())
				i = Fonts_.insert(std::make_pair(id, Create(font, height))).first;
			return i->second;
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

		Font(vterm::Font font, unsigned width, unsigned height, T const & handle) :
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
		static Font * Create(vterm::Font font, unsigned height);

		static std::unordered_map<unsigned, Font *> Fonts_;
	};


	template<typename T>
	std::unordered_map<unsigned, Font<T> *> Font<T>::Fonts_;


	/** Description of settings relevant for terminal windows. 

	 */
	class TerminalSettings {
	public:

		/** Basic title for the terminal window. 
		 */
		std::string defaultName = "terminal++";

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

	}; // tpp::TerminalSettings


	/** Single terminal window.

	    The terminal window is a vterm renderer that can display the contents of the associated terminal. This class provides the common, platform independent functionality. 
	 */
	class BaseTerminalWindow : public vterm::Renderer {
	public:

		TerminalSettings * settings() const {
			return settings_;
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

		// methods --------------------------------------------------------------------------------------

		virtual void show() = 0;
		virtual void hide() = 0;

	protected:

		/** Because the blink attribute has really nothing to do with the font itself, this simple functions strips its value from given font so that fonts excluding the blinking can be easily compared. */
		static vterm::Font DropBlink(vterm::Font font) {
			font.setBlink(false);
			return font;
		}

		BaseTerminalWindow(TerminalSettings * settings) :
			vterm::Renderer(settings->defaultCols, settings->defaultRows),
			settings_(settings),
			name_(settings->defaultName),
			widthPx_(settings->defaultFontWidth * settings->defaultCols),
			heightPx_(settings->defaultFontHeight * settings->defaultRows),
			zoom_(settings->defaultZoom),
			cellWidthPx_(settings->defaultFontWidth),
			cellHeightPx_(settings->defaultFontHeight) {

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

		virtual void doTitleChange(vterm::VT100::TitleEvent & e) = 0;

		TerminalSettings * settings_;

		std::string name_;

		/** Width and height of the window client area in pixels. 
		 */
		unsigned widthPx_;
		unsigned heightPx_;

		/** Zoom level of the window. 
		 */
		double zoom_;

		/** Width of a single cell in pixels. 
		 */
		unsigned cellWidthPx_;

		/** Height of a single cell in pixels. 
		 */
		unsigned cellHeightPx_;

	};


} // namespace tpp
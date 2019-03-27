#pragma once

#include "vterm/renderer.h"

namespace tpp {

	/** Description of settings relevant for terminal windows. 

	 */
	class TerminalSettings {
	public:

		/** Basic title for the terminal window. 
		 */
		std::string defaultName;

		/** Default width and height of the terminal display (in terminal rows and cols, not in pixels). 
		 */
		unsigned defaultCols;
		unsigned defaultRows;

		/** Name of the font to be used by the terminal. */
		std::string font;

		/** Default width and height in pixels, of the selected font. I.e. the width and height of a single terminal cell. */
		unsigned defaultFontHeight;
		unsigned defaultFontWidth;

		/** Default zoom of the window. 
		 */
		double defaultZoom;

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

		BaseTerminalWindow(TerminalSettings * settings) :
			vterm::Renderer(settings->defaultCols, settings->defaultRows),
			settings_(settings),
			name_(settings->defaultName),
			widthPx_(cellWidthPx_ * settings->defaultCols),
			heightPx_(cellHeightPx_ * settings->defaultRows),
			zoom_(settings->defaultZoom),
			cellWidthPx_(settings->defaultFontWidth),
			cellHeightPx_(settings->defaultFontHeight) {

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
			// update width & height of the cell
			cellWidthPx_ = settings_->defaultFontWidth * value;
			cellHeightPx_ = settings_->defaultFontHeight * value;
			NOT_IMPLEMENTED;
			// resize the terminal properly
		}

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
#pragma once

#include "vterm/vterm.h"

namespace tpp {

	/** Minimal information about font that is required at the level of the GUI renderer. 
	 */
	class Font {
	public:
		vterm::Font const & font() const {
			return font_;
		}

		unsigned width() const {
			return width_;
		}

		unsigned height() const {
			return height_;
		}

	protected:
		Font(vterm::Font const & font,  unsigned width, unsigned height) :
			font_(font),
			width_(width),
			height_(height) {
			ASSERT(width != 0 && height != 0) << "Fonts of zero sizes not supported.";
		}

		vterm::Font font_;
		unsigned width_;
		unsigned height_;
	};

	/** Generic class for graphical terminal renderers. 
	
	    Extends the terminal's Renderer with some GUI specific information. 
	 */
	class TerminalWindow : public vterm::VTerm::Renderer {
	public:
		/** Width of the renderer's client area in pixels. 
		 */
		unsigned width() const {
			return width_;
		}

		/** Height of the renderer's client area in pixels. 
		 */
		unsigned height() const {
			return height_;
		}

		virtual void show() = 0;
	protected:

		/** Default values for GUI renderer. 
		 */
		TerminalWindow() :
			width_(0),
			height_(0),
			fontWidth_(10),
			fontHeight_(18) {
		}

		/** Called when the renderer's client area resizes so that the contents could be updated. 

		    Updates the width and height properties (in pixels) and then resizes the underlying terminal according to the selected font width and height.
		 */
		virtual void resize(unsigned width, unsigned height) {
			width_ = width;
			height_ = height;
			resizeTerminal(width / fontWidth_, height / fontHeight_);
		}


		unsigned width_;
		unsigned height_;

		unsigned fontWidth_;
		unsigned fontHeight_;

	};

} // namespace tpp
#pragma once
#if (defined ARCH_UNIX)

#include "helpers/helpers.h"

#include "x11.h"

#include "../font.h"
#include "x11_application.h"

namespace tpp {

	/** Because XFT font sizes are ascent only, the font is obtained by trial and error. First we try the requested height and then, based on the actual height. If the actually obtained height differs, height multiplier is calculated and the font is re-obtained with the height adjusted. */
	template<>
	inline Font<XftFont*>* Font<XftFont*>::Create(ui::Font font, unsigned height) {
		X11Application* app = X11Application::Instance();
		// get the name of the font we want w/o the actual height
		std::string fName = *config::FontFamily;
		if (font.bold())
			fName += ":bold";
		if (font.italics())
			fName += ":italic";
		fName += ":pixelsize=";
		// get the font we request
		std::string fRequest = STR(fName << height);
		XftFont* handle = XftFontOpenName(app->xDisplay(), app->xScreen(), fRequest.c_str());
		// if the size is not correct, adjust height and try again
		if (static_cast<unsigned>(handle->ascent + handle->descent) != height) {
			double hAdj = height * static_cast<double>(height) / (handle->ascent + handle->descent);
			XftFontClose(app->xDisplay(), handle);
			fRequest = STR(fName << hAdj);
			handle = XftFontOpenName(app->xDisplay(), app->xScreen(), fRequest.c_str());
		}
		// get the font width
		XGlyphInfo gi;
		XftTextExtentsUtf8(app->xDisplay(), handle, (FcChar8*)"m", 1, &gi);
		// return the font 
        Font<XftFont*>* result = new Font<XftFont*>(
            font,
            gi.width,
            height,
            handle->ascent,
            handle
        );
        // add underline and strikethrough metrics
		result->underlineOffset_ = result->ascent_ + 1;
		result->underlineThickness_ = 1;
		result->strikethroughOffset_ = result->ascent_ * 2 / 3;
		result->strikethroughThickness_ = 1;
        return result;
	}


} // namespace tpp

#endif
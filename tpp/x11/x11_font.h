#pragma once
#if (defined ARCH_UNIX)

#include "helpers/helpers.h"

#include "x11.h"

#include "../font.h"
#include "x11_application.h"

#include "../config.h"

namespace tpp {

	/** Because XFT font sizes are ascent only, the font is obtained by trial and error. First we try the requested height and then, based on the actual height. If the actually obtained height differs, height multiplier is calculated and the font is re-obtained with the height adjusted. */
	template<>
	inline Font<XftFont*>* Font<XftFont*>::Create(ui::Font font, unsigned cellWidth, unsigned cellHeight) {
		X11Application* app = X11Application::Instance();
		// update the cell size accordingly
		cellWidth *= font.width();
		cellHeight *= font.height();
		// get the name of the font we want w/o the actual height
		std::string fName = font.doubleWidth() ? Config::Instance().doubleWidthFontFamily() : Config::Instance().fontFamily();
		if (font.bold())
			fName += ":bold";
		if (font.italics())
			fName += ":italic";
		fName += ":pixelsize=";
		double xFontHeight = cellHeight;
		// get the font we request
		std::string fRequest = STR(fName << xFontHeight);
		XftFont* handle = XftFontOpenName(app->xDisplay(), app->xScreen(), fRequest.c_str());
		// if the size is not correct, adjust height and try again
		if (static_cast<unsigned>(handle->ascent + handle->descent) != cellHeight) {
			xFontHeight = xFontHeight * xFontHeight / (handle->ascent + handle->descent);
			XftFontClose(app->xDisplay(), handle);
			fRequest = STR(fName << xFontHeight);
			handle = XftFontOpenName(app->xDisplay(), app->xScreen(), fRequest.c_str());
		}
		// get the font width
		XGlyphInfo gi;
		XftTextExtentsUtf8(app->xDisplay(), handle, (FcChar8*)"m", 1, &gi);
		// update the font width and height accordingly
		unsigned offsetLeft = 0;
		unsigned offsetTop = 0;
		unsigned fontHeight = cellHeight;
		unsigned fontWidth = gi.width;
		// if cellWidth is 0, the centering is ignored (the fontWidth will be used as default cell width)
		if (cellWidth != 0) {
			// if the width is greater then allowed, decrease the font size accordingly and center vertically
			if (fontWidth > cellWidth) {
				float x = static_cast<float>(cellWidth) / fontWidth;
				xFontHeight *= x;
				fontHeight = static_cast<unsigned>(fontHeight * x);
				fontWidth = cellWidth;
				offsetTop = (cellHeight - fontHeight) / 2;
				XftFontClose(app->xDisplay(), handle);
				fRequest = STR(fName << xFontHeight);
				handle = XftFontOpenName(app->xDisplay(), app->xScreen(), fRequest.c_str());
			// otherwise center horizontally
			} else {
				offsetLeft = (cellWidth - fontWidth) / 2;
			}
		}
		// return the font 
        Font<XftFont*>* result = new Font<XftFont*>(
            font,
            fontWidth,
			fontHeight,
			offsetLeft,
			offsetTop,
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
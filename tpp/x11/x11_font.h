#pragma once
#if (defined ARCH_UNIX)

#include "helpers/helpers.h"

#include "x11.h"

#include "../font.h"
#include "x11_application.h"

#include "../config.h"

namespace tpp {

    class X11Font : public Font<X11Font> {
    public:
        XftFont * xftFont() const {
            return xftFont_;
        }

        ~X11Font() override {
            XftFontClose(X11Application::Instance()->xDisplay(), xftFont_);
            FcPatternDestroy(pattern_);
        }

        bool supportsCodepoint(char32_t codepoint) override {
            return XftCharIndex(X11Application::Instance()->xDisplay(), xftFont_, codepoint) != 0;
        }

    private:
        friend class Font<X11Font>;

        X11Font(ui::Font font, unsigned cellWidth, unsigned cellHeight):
            Font<X11Font>(font) {
            // update the cell size accordingly
            cellWidth *= font.width();
            cellHeight *= font.height();
            // get the font pattern 
            pattern_ = FcPatternCreate();
            FcPatternAddBool(pattern_, FC_SCALABLE, FcTrue);
            FcPatternAddString(pattern_, FC_FAMILY, helpers::pointer_cast<FcChar8 const *>(font.doubleWidth() ? Config::Instance().doubleWidthFontFamily().c_str() : Config::Instance().fontFamily().c_str()));
            FcPatternAddInteger(pattern_, FC_WEIGHT, font.bold() ? FC_WEIGHT_BOLD : FC_WEIGHT_NORMAL);
            FcPatternAddInteger(pattern_, FC_SLANT, font.italics() ? FC_SLANT_ITALIC : FC_SLANT_ROMAN);
            FcPatternAddDouble(pattern_, FC_PIXEL_SIZE, cellHeight);
            initializeFromPattern(cellWidth, cellHeight);
        }

        X11Font(X11Font const & from, unsigned cellWidth, unsigned cellHeight, char32_t requiredCodepoint):
            Font<X11Font>(from.font_) {
            cellWidth *= font_.width();
            cellHeight *= font_.height();
            // get the font pattern 
            pattern_ = FcPatternDuplicate(from.pattern_);
            FcPatternRemove(pattern_, FC_FAMILY, 0);
            FcPatternRemove(pattern_, FC_PIXEL_SIZE, 0);
            FcPatternAddDouble(pattern_, FC_PIXEL_SIZE, cellHeight);
            FcCharSet * charSet = FcCharSetCreate();
            FcCharSetAddChar(charSet, requiredCodepoint);
            FcPatternAddCharSet(pattern_, FC_CHARSET, charSet);
            initializeFromPattern(cellWidth, cellHeight);
        }

        void initializeFromPattern(unsigned cellWidth, unsigned cellHeight) {
            X11Application * app = X11Application::Instance();
            double fontHeight = cellHeight;
            xftFont_ = MatchFont(pattern_);
            // if the font height is not the cellHeight, update the pixel size accordingly and get the font again
            if (static_cast<unsigned>(xftFont_->ascent + xftFont_->descent) != cellHeight) {
                fontHeight = fontHeight * fontHeight / (xftFont_->ascent + xftFont_->descent);
                XftFontClose(app->xDisplay(), xftFont_);
                FcPatternRemove(pattern_, FC_PIXEL_SIZE, 0);
                FcPatternAddDouble(pattern_, FC_PIXEL_SIZE, fontHeight);
                xftFont_ = MatchFont(pattern_);
            }
            // now calculate the width of the font 
            XGlyphInfo gi;
            XftTextExtentsUtf8(app->xDisplay(), xftFont_, (FcChar8*)"M", 1, &gi);
            // update the font width and height accordingly
            heightPx_ = cellHeight;
            widthPx_ = gi.width;
            // if cellWidth is 0, then the font is constructed to later determine the width of the cell and therefore no width adjustments are needed
            if (cellWidth != 0) {
                // if the width is greater than the cell width, we need smaller font
                if (widthPx_ > cellWidth) {
                    double x = static_cast<double>(cellWidth) / widthPx_;
                    fontHeight = fontHeight * x;
                    widthPx_ = cellWidth;
                    heightPx_ = static_cast<unsigned>(heightPx_ * x);
                    XftFontClose(app->xDisplay(), xftFont_);
                    FcPatternRemove(pattern_, FC_PIXEL_SIZE, 0);
                    FcPatternAddDouble(pattern_, FC_PIXEL_SIZE, fontHeight);
                    xftFont_ = MatchFont(pattern_);
					offsetTop_ = (cellHeight - heightPx_) / 2;
                // if the width is smaller, center the glyph horizontally
                } else {
                    offsetLeft_ = (cellWidth - widthPx_) / 2;
                }
            }
            // now that we have correct font, initialize the rest of the properties
            ascent_ = xftFont_->ascent;
            // add underline and strikethrough metrics
            underlineOffset_ = ascent_ + 1;
            underlineThickness_ = 1;
            strikethroughOffset_ = ascent_ * 2 / 3;
            strikethroughThickness_ = 1;
        }

        XftFont * xftFont_;
        FcPattern * pattern_;

        static XftFont * MatchFont(FcPattern * pattern) {
            X11Application * app = X11Application::Instance();
            FcPattern * configured = FcPatternDuplicate(pattern);
            if (configured == nullptr)
                return nullptr;
            FcConfigSubstitute(nullptr, configured, FcMatchPattern);
            XftDefaultSubstitute(app->xDisplay(), app->xScreen(), configured);
            FcResult fcr;
            FcPattern * matched = FcFontMatch(nullptr, configured, & fcr);
            if (matched == nullptr) {
                FcPatternDestroy(configured);
                return nullptr;
            }
            XftFont * font = XftFontOpenPattern(app->xDisplay(), matched);
            FcPatternDestroy(configured);
            FcPatternDestroy(matched);
            return font;
        }

    };

} // namespace tpp

#endif
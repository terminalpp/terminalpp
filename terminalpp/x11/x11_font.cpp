#if (defined ARCH_LINUX && defined RENDERER_NATIVE)
#include <cmath>

#include "x11_font.h"

namespace tpp {

    std::unordered_map<XftFont*, unsigned> X11Font::ActiveFontsMap_;
    
    X11Font::X11Font(ui::Font font, int cellHeight, int cellWidth):
        Font<X11Font>{font, ui::Size{cellWidth, cellHeight}} {
        // update the cell size accordingly
        // get the font pattern 
        pattern_ = FcPatternCreate();
        FcPatternAddBool(pattern_, FC_SCALABLE, FcTrue);
        FcPatternAddString(pattern_, FC_FAMILY, pointer_cast<FcChar8 const *>(tpp::Config::Instance().familyForFont(font).c_str()));
        FcPatternAddInteger(pattern_, FC_WEIGHT, font.bold() ? FC_WEIGHT_BOLD : FC_WEIGHT_NORMAL);
        FcPatternAddInteger(pattern_, FC_SLANT, font.italic() ? FC_SLANT_ITALIC : FC_SLANT_ROMAN);
        FcPatternAddDouble(pattern_, FC_PIXEL_SIZE, fontSize_.height());
        initializeFromPattern();
    } 

    X11Font::X11Font(X11Font const & base, char32_t codepoint):
        Font<X11Font>{base.font_, base.fontSize_} {
        // get the font pattern 
        pattern_ = FcPatternDuplicate(base.pattern_);
        FcPatternRemove(pattern_, FC_FAMILY, 0);
        FcPatternRemove(pattern_, FC_PIXEL_SIZE, 0);
        FcPatternAddDouble(pattern_, FC_PIXEL_SIZE, fontSize_.height());
        FcCharSet * charSet = FcCharSetCreate();
        FcCharSetAddChar(charSet, codepoint);
        FcPatternAddCharSet(pattern_, FC_CHARSET, charSet);
        initializeFromPattern();

    }

    void X11Font::initializeFromPattern() {
        X11Application * app = X11Application::Instance();
        double fontHeight = fontSize_.height();
        xftFont_ = MatchFont(pattern_);
        if (xftFont_ == nullptr) {
            FcValue fontName;
            FcPatternGet(pattern_, FC_FAMILY, 0, &fontName);
            app->alert(STR("Unable to load font family " << fontName.u.s << ", trying fallback"));
            // for the fallback, just remove the font family from the pattern and see if we get *any* font
            FcPatternDel(pattern_, FC_FAMILY);
            xftFont_ = MatchFont(pattern_);
            OSCHECK(xftFont_ != nullptr) << "Unable to initialize fallback font.";
        }
        // if the font height is not the cellHeight, update the pixel size accordingly and get the font again
        if (static_cast<int>(xftFont_->ascent + xftFont_->descent) != fontSize_.height()) {
            fontHeight = std::floor(fontHeight * fontHeight / (xftFont_->ascent + xftFont_->descent));
            CloseFont(xftFont_);
            FcPatternRemove(pattern_, FC_PIXEL_SIZE, 0);
            FcPatternAddDouble(pattern_, FC_PIXEL_SIZE, fontHeight);
            xftFont_ = MatchFont(pattern_);
        }
        // now calculate the width of the font 
        XGlyphInfo gi;
        XftTextExtentsUtf8(app->xDisplay_, xftFont_, (FcChar8*)"M", 1, &gi);
        // update the font width and height accordingly
        int h = fontSize_.height();
        int w = gi.xOff; 
        // if cellWidth is 0, then the font is constructed to later determine the width of the cell and therefore no width adjustments are needed
        if (fontSize_.width() == 0) {
            fontSize_.setWidth(w);
            offset_ = ui::Point{0,0};
        // if the width is smaller, center the glyph horizontally
        } else if (w < fontSize_.width()) {
            offset_.setX((fontSize_.width() - w) / 2);
        // if the width is greater than the cell width, we need smaller font
        } else {
            double x = static_cast<double>(fontSize_.width()) / w;
            fontHeight *=  x;
            h = static_cast<unsigned>(h * x);
            CloseFont(xftFont_);
            FcPatternRemove(pattern_, FC_PIXEL_SIZE, 0);
            FcPatternAddDouble(pattern_, FC_PIXEL_SIZE, fontHeight);
            xftFont_ = MatchFont(pattern_);
            offset_.setY((fontSize_.height() - h) / 2);
        }
        // now that we have correct font, initialize the rest of the properties
        ascent_ = xftFont_->ascent;
        // add underline and strikethrough metrics
        underlineOffset_ = ascent_ + 1;
        underlineThickness_ = font_.size();
        strikethroughOffset_ = ascent_ * 2 / 3;
        strikethroughThickness_ = font_.size();
    }

    XftFont * X11Font::MatchFont(FcPattern * pattern) {
        X11Application * app = X11Application::Instance();
        FcPattern * configured = FcPatternDuplicate(pattern);
        if (configured == nullptr)
            return nullptr;
        FcConfigSubstitute(nullptr, configured, FcMatchPattern);
        XftDefaultSubstitute(app->xDisplay_, app->xScreen_, configured);
        FcResult fcr;
        FcPattern * matched = FcFontMatch(nullptr, configured, & fcr);
        if (matched == nullptr) {
            FcPatternDestroy(configured);
            return nullptr;
        }
        XftFont * font = XftFontOpenPattern(app->xDisplay_, matched);
        auto i = ActiveFontsMap_.find(font);
        if (i == ActiveFontsMap_.end())
            ActiveFontsMap_.insert(std::make_pair(font, 1));
        else 
            ++(i->second);
        FcPatternDestroy(configured);
        return font;
    }


    void X11Font::CloseFont(XftFont * font) {
        auto i = ActiveFontsMap_.find(font);
        ASSERT(i != ActiveFontsMap_.end());
        if (i->second == 1) {
            ActiveFontsMap_.erase(i);
            XftFontClose(X11Application::Instance()->xDisplay_, font);
        } else {
            --(i->second);
        }
    }

} // namespace tpp

#endif
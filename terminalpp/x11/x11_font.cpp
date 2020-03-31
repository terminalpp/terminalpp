#include "x11_font.h"

namespace tpp2 {

    std::unordered_map<XftFont*, unsigned> X11Font::ActiveFontsMap_;
    
    X11Font::X11Font(ui2::Font font, int cellHeight, int cellWidth):
        Font<X11Font>{font, cellHeight, cellWidth} {
        // get the font pattern 
        pattern_ = FcPatternCreate();
        FcPatternAddBool(pattern_, FC_SCALABLE, FcTrue);
        FcPatternAddString(pattern_, FC_FAMILY, pointer_cast<FcChar8 const *>(font.doubleWidth() ? tpp::Config::Instance().font.doubleWidthFamily().c_str() : tpp::Config::Instance().font.family().c_str()));
        FcPatternAddInteger(pattern_, FC_WEIGHT, font.bold() ? FC_WEIGHT_BOLD : FC_WEIGHT_NORMAL);
        FcPatternAddInteger(pattern_, FC_SLANT, font.italic() ? FC_SLANT_ITALIC : FC_SLANT_ROMAN);
        FcPatternAddDouble(pattern_, FC_PIXEL_SIZE, cellHeight);
        //initializeFromPattern(cellWidth, cellHeight);
    } 

    X11Font::X11Font(X11Font const & base, char32_t codepoint):
        Font<X11Font>{base.font_, base.cellHeight_, base.cellWidth_} {

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

} // namespace tpp2
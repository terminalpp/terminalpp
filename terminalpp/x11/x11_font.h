#pragma once
#if (defined ARCH_UNIX && defined RENDERER_NATIVE)

#include <unordered_map>

#include "helpers/helpers.h"

#include "x11.h"

#include "../font.h"
#include "x11_application.h"

#include "../config.h"

namespace tpp {

    class X11Font : public Font<X11Font> {
    public:

        ~X11Font() override {
            CloseFont(xftFont_);
            FcPatternDestroy(pattern_);
        }

        XftFont * xftFont() const {
            return xftFont_;
        }

        bool supportsCodepoint(char32_t codepoint) {
            return XftCharIndex(X11Application::Instance()->xDisplay_, xftFont_, codepoint) != 0;
        }

    private:
        friend class Font<X11Font>;

        X11Font(ui::Font font, int cellHeight, int cellWidth = 0);

        X11Font(X11Font const & base, char32_t codepoint);

        void initializeFromPattern();

        XftFont * xftFont_;
        FcPattern * pattern_;

        static XftFont * MatchFont(FcPattern * pattern);

        static void CloseFont(XftFont * font);

        static std::unordered_map<XftFont*, unsigned> ActiveFontsMap_;

    };


} // namespace tpp

#endif
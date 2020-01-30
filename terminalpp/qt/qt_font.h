#pragma once
#if (defined RENDERER_QT)

#include "../font.h"
#include "qt_application.h"

namespace tpp {

    class QtFont : public Font<QtFont> {
    public:
		bool supportsCodepoint(char32_t codepoint) override {
            MARK_AS_UNUSED(codepoint);
            NOT_IMPLEMENTED;
		}

    private:
        friend class Font<QtFont>;

        QtFont(ui::Font font, unsigned cellWidth, unsigned cellHeight): 
            Font<QtFont>(font) {
            MARK_AS_UNUSED(cellWidth);
            MARK_AS_UNUSED(cellHeight);
        }
    };

} // namespace tpp


#endif
#pragma once
#if (defined RENDERER_QT)

#include "../font.h"
#include "../config.h"
#include "qt_application.h"


namespace tpp {

    class QtFont : public Font<QtFont> {
    public:
		bool supportsCodepoint(char32_t codepoint) override {
            MARK_AS_UNUSED(codepoint);
            NOT_IMPLEMENTED;
		}

        QFont const& font() const {
            return font_;
        }

    private:
        friend class Font<QtFont>;

        /** Determines the font family to be used for given font from the configuration. 
         */
        static QString FontFamily(ui::Font font) {
            Config const & config{Config::Instance()};
            return font.doubleWidth() ? config.font.doubleWidthFamily().c_str() : config.font.family().c_str();
        }

        QtFont(ui::Font font, unsigned cellWidth, unsigned cellHeight): 
            Font<QtFont>{font} {
            Config const & config{Config::Instance()};
            widthPx_ = cellWidth * font.width();
            heightPx_ = cellHeight * font.height();
            font_.setFamily(font.doubleWidth() ? config.font.doubleWidthFamily().c_str() : config.font.family().c_str());
            if (font.bold())
                font_.setBold(true);
            if (font.italics())
                font_.setItalic(true);
            font_.setPixelSize(heightPx_);
            QFontMetrics metrics{font_};
            QRect brect{metrics.boundingRect('M')};
            widthPx_ = brect.width();
            heightPx_ = brect.height();
        }

        QFont font_;
    };

} // namespace tpp


#endif
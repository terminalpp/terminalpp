#pragma once
#if (defined RENDERER_QT)

#include "../font.h"
#include "../config.h"
#include "qt_application.h"


namespace tpp {

    class QtFont : public Font<QtFont> {
    public:
		bool supportsCodepoint(char32_t codepoint) override {
            return QFontMetrics{font_}.inFontUcs4(codepoint);
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
// TODO delete this, only on windows where QT expects different font names
#if (defined ARCH_WINDOWS)
            font_.setFamily("Iosevka NF");
#endif
            if (font.bold())
                font_.setBold(true);
            if (font.italics())
                font_.setItalic(true);
            font_.setPixelSize(heightPx_);
            QFontMetrics metrics{font_};
            // scale the font if the ascent and descent are 
            unsigned h = static_cast<unsigned>(metrics.ascent() + metrics.descent());
            if (h != heightPx_) {
                h = static_cast<unsigned>(heightPx_ * (heightPx_ / static_cast<double>(h)));
                font_.setPixelSize(h);
                metrics = QFontMetrics{font_};
            }
            //QRect brect{metrics.boundingRect('M')};
            //widthPx_ = brect.width();
// TODO older versions if QT should not be supported in the future, this is for simple builds on Ubuntu for now
#if (QT_VERSION >= QT_VERSION_CHECK(5,11,0))
            widthPx_ = static_cast<unsigned>(metrics.horizontalAdvance('M'));
#else
            widthPx_ = metrics.width('M');
#endif
            ascent_ = metrics.ascent();
            underlineOffset_ = ascent_ + 1;
            underlineThickness_ = 1;
            strikethroughOffset_ = ascent_ * 2 / 3;
            strikethroughThickness_ = 1;
        }

        QFont font_;
    };

} // namespace tpp


#endif
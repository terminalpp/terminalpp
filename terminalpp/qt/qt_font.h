#pragma once
#if (defined RENDERER_QT)

#include "../font.h"
#include "../config.h"
#include "qt_application.h"

namespace tpp {

    /** QT Font Wrapper.
     
        Since Qt fonts do all the work (such as font fallback) themselves, the wrapper can be very minimal.
     */
    class QtFont : public Font<QtFont> {
    public:
        QFont const & qFont() const {
            return qFont_;
        }

    protected:
        friend class Font<QtFont>;

        QtFont(ui::Font font, int cellHeight, int cellWidth = 0):
            Font<QtFont>{font, cellHeight, cellWidth} {
            tpp::Config const & config{tpp::Config::Instance()};
            qFont_.setFamily(font.doubleWidth() ? config.font.doubleWidthFamily().c_str() : config.font.family().c_str());
            if (font.bold())
                qFont_.setBold(true);
            if (font.italic())
                qFont_.setItalic(true);
            qFont_.setPixelSize(cellHeight_);
            QFontMetrics metrics{qFont_};
            // scale the font if the ascent and descent are 
            int h = static_cast<int>(metrics.ascent() + metrics.descent());
            if (h != cellHeight_) {
                h = static_cast<int>(cellHeight_ * (cellHeight_ / static_cast<double>(h)));
                qFont_.setPixelSize(h);
                metrics = QFontMetrics{qFont_};
            }
            //QRect brect{metrics.boundingRect('M')};
            //widthPx_ = brect.width();
// TODO older versions if QT should not be supported in the future, this is for simple builds on Ubuntu for now
// TODO should deal with keeping the font size and centering the font too
#if (QT_VERSION >= QT_VERSION_CHECK(5,11,0))
            cellWidth_ = static_cast<unsigned>(metrics.horizontalAdvance('M'));
#else
            cellWidth_ = metrics.width('M');
#endif
            ascent_ = metrics.ascent();
            underlineOffset_ = ascent_ + 1;
            underlineThickness_ = 1;
            strikethroughOffset_ = ascent_ * 2 / 3;
            strikethroughThickness_ = 1;
        }

        QFont qFont_;

    }; // tpp::QtFont


} // namespace tpp

#endif
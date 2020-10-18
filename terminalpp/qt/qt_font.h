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
            Font<QtFont>{font, ui::Size{cellWidth, cellHeight}} {
            tpp::Config const & config{tpp::Config::Instance()};
            qFont_.setFamily(font.doubleWidth() ? config.renderer.font.doubleWidthFamily().c_str() : config.renderer.font.family().c_str());
            if (font.bold())
                qFont_.setBold(true);
            if (font.italic())
                qFont_.setItalic(true);
            qFont_.setPixelSize(cellSize_.height());
            QFontMetrics metrics{qFont_};
            // scale the font if the ascent and descent are 
            int h = static_cast<int>(metrics.ascent() + metrics.descent());
            if (h != cellSize_.height()) {
                h = static_cast<int>(cellSize_.height() * (cellSize_.height() / static_cast<double>(h)));
                qFont_.setPixelSize(h);
                metrics = QFontMetrics{qFont_};
            }
// TODO older versions if QT should not be supported in the future, this is for simple builds on Ubuntu for now
// TODO should deal with keeping the font size and centering the font too
#if (QT_VERSION >= QT_VERSION_CHECK(5,11,0))
            int w = static_cast<unsigned>(metrics.horizontalAdvance('M'));
#else
            int w = metrics.width('M');
#endif
            if (cellSize_.width() == 0) {
                cellSize_.setWidth(w);
                offset_ = ui::Point{0,0};
            } else if (w <= cellSize_.width()) {
                offset_.setX((cellSize_.width() - w) / 2);
            } else {
                float x = static_cast<float>(cellSize_.width()) / w;
                h = static_cast<int>(h * x);
                qFont_.setPixelSize(h);
                metrics = QFontMetrics{qFont_};
                offset_.setY((cellSize_.height() - h) / 2);
            }
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
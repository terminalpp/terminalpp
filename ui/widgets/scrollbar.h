#pragma once

#include "../canvas.h"

namespace ui {

    /** Scrollbar implementation. 

        Provides basic functionality of a vertical or horizontal scrollbar.
     */
    class Scrollbar {
    public:
        Scrollbar():
            min_{0},
            max_{100},
            pos_{0},
            size_{1} {
        }

        void resize(int min, int max) {
            ASSERT(max >= min);
            min_ = min;
            max_ = max;
            if (pos_ < min)
                pos_ = min;
            if (pos_ > max)
                pos_ = max;
            if (size_ > max_ - pos_)
                size_ = max_ - pos_;
        }

        void setSize(int size) {
            ASSERT(size_ <= max_ - min_);
            size_ = size;
        }

        void setPosition(int pos) {
            ASSERT(pos + size_ <= max_);
            pos_ = pos;
        }

        /** Draws a vertical scrollbar with a given height from selected start. 
         */
        virtual void drawVertical(Canvas & canvas, Point p, int height) = 0;

    protected:
        /** Calculates the placement of the actual scrollbar wrt given start and max height. 
         */
        std::pair<Point, int> getBarPlacement(Point start, int height) {
            int barHeight = std::max(1, height * size_ / (max_ - min_));
            int barY = start.y + height * (pos_ - min_) / (max_ - min_);
            if (barY + barHeight > height)
                barY = start.y + height - barHeight;
            return std::make_pair(Point(start.x, barY), barHeight);
        }

    private:
        int min_;
        int max_;
        int pos_;
        int size_;

        bool active_;


    };

    class ScrollbarFlat : public Scrollbar {
    public:
        ScrollbarFlat():
            color(Color::Gray().setAlpha(128)) {
        }

        Color color;

        void drawVertical(Canvas & canvas, Point p, int height) override {
            canvas.borderLineRight(p, height, color, false);
            std::pair<Point, int> barPlacement{getBarPlacement(p, height)};
            canvas.borderLineRight(barPlacement.first, barPlacement.second, color, true);
        };

    };



} // namespace ui
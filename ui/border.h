#pragma once

#include "color.h"

namespace ui {

    class Border {
    public:

        enum class Kind {
            None, 
            Thin,
            Thick,
        }; // ui::Border::Kind

        /** Creates empty border with no specified color (Color::None). 
         
            This is just the default constructor, for creating actual borders, the static constructors should be used. 
         */
        Border():
            color_{Color::None},
            border_{0} {
        }

        static Border Empty(Color color = Color::None) {
            return Border{}.setColor(color);
        }

        static Border All(Color color, Kind kind) {
            return Border::Empty(color).setLeft(kind).setRight(kind).setTop(kind).setBottom(kind);
        }

        bool empty() const {
            return border_ == 0;
        }

        Border & setAll(Kind kind) {
            setLeft(kind);
            setRight(kind);
            setTop(kind);
            setBottom(kind);
            return *this;
        }

        Border & clear() {
            border_ = 0;
            return *this;
        }

        Color color() const {
            return color_;
        }

        Border & setColor(Color color) {
            color_ = color;
            return *this;
        }

        Kind left() const {
            return static_cast<Kind>((border_ >> LEFT) & MASK);
        }

        Kind right() const {
            return static_cast<Kind>((border_ >> RIGHT) & MASK);
        }

        Kind top() const {
            return static_cast<Kind>((border_ >> TOP) & MASK);
        }

        Kind bottom() const {
            return static_cast<Kind>((border_ >> BOTTOM) & MASK);
        }

        Border & setLeft(Kind kind) {
            border_ = (border_ & (~ (MASK << LEFT))) | (static_cast<uint8_t>(kind) << LEFT); 
            return *this;
        }

        Border & setRight(Kind kind) {
            border_ = (border_ & (~ (MASK << RIGHT))) | (static_cast<uint8_t>(kind) << RIGHT); 
            return *this;
        }

        Border & setTop(Kind kind) {
            border_ = (border_ & (~ (MASK << TOP))) | (static_cast<uint8_t>(kind) << TOP); 
            return *this;
        }

        Border & setBottom(Kind kind) {
            border_ = (border_ & (~ (MASK << BOTTOM))) | (static_cast<uint8_t>(kind) << BOTTOM); 
            return *this;
        }

        bool operator == (Border const & other) const {
            return color_ == other.color_ && border_ == other.border_;
        }

        bool operator != (Border const & other) const {
            return color_ != other.color_ || border_ != other.border_;
        }

        Border operator + (Border const & other) const {
            Border result{*this};
            result.setColor(other.color());
            if (other.left() != Kind::None)
                result.setLeft(other.left());
            if (other.right() != Kind::None)
                result.setRight(other.right());
            if (other.top() != Kind::None)
                result.setTop(other.top());
            if (other.bottom() != Kind::None)
                result.setBottom(other.bottom());
            return result;
        }

    private:
        static constexpr uint16_t MASK = 0x03;
        static constexpr uint16_t LEFT = 0;
        static constexpr uint16_t RIGHT = 2;
        static constexpr uint16_t TOP = 4;
        static constexpr uint16_t BOTTOM = 6;

        Color color_;
        uint8_t border_;

    }; // ui::Border

}
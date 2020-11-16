#pragma once

#include "helpers/helpers.h"
#include "helpers/bits.h"

namespace ui {

    /** Font properties. 
     
        Contains the information about the font size, type and decorations. 
     */
    class Font {
    public:

        Font():
            font_{0} {
        }

        bool bold() const {
            return font_ & BOLD;
        }

        Font & setBold(bool value = true) {
            font_ = SetBit(font_, BOLD, value);
            return *this;
        }

        bool italic() const {
            return font_ & ITALIC;
        }

        Font & setItalic(bool value = true) {
            font_ = SetBit(font_, ITALIC, value);
            return *this;
        }

        bool underline() const {
            return font_ & UNDERLINE;
        }

        Font & setUnderline(bool value = true) {
            font_ = SetBit(font_, UNDERLINE, value);
            return *this;
        }

        /** If true, underline and strikethrough are drawn dashed, otherwise the line is solid. 
         */
        bool dashed() const {
            return font_ & DASHED;
        }

        /** If true, underline and strikethrough are drawn dashed, otherwise the line is solid. 
         */
        Font & setDashed(bool value = true) {
            font_ = SetBit(font_, DASHED, value);
            return *this;
        }

        bool strikethrough() const {
            return font_ & STRIKETHROUGH;
        }

        Font & setStrikethrough(bool value = true) {
            font_ = SetBit(font_, STRIKETHROUGH, value);
            return *this;
        }

        bool blink() const {
            return font_ & BLINK;
        }

        Font & setBlink(bool value = true) {
            font_ = SetBit(font_, BLINK, value);
            return *this;
        }

        bool doubleWidth() const {
            return font_ & DOUBLE_WIDTH;
        }

        Font & setDoubleWidth(bool value = true) {
            font_ = SetBit(font_, DOUBLE_WIDTH, value);
            return *this;
        }

        int size() const {
            return (font_ & SIZE_MASK) + 1;
        }

        Font & setSize(int size) {
            size -= 1;
            ASSERT(size >= 0 && size < 8);
            font_ = SetBits(font_, SIZE_MASK, static_cast<uint16_t>(size));
            return *this;
        }

        int width() const {
            return doubleWidth() ? size() * 2 : size();
        }

        int height() const {
            return size();
        }

        bool operator == (Font const & other) const {
            return font_ == other.font_;
        } 

        bool operator != (Font const & other) const {
            return font_ != other.font_;
        }

        /** Adds any attributes specified for the other cell, leaving everything else inact. 
         */
        Font & orAttributesFrom(Font const & from) {
            font_ = font_ | (from.font_ & (BOLD | ITALIC | UNDERLINE | STRIKETHROUGH | BLINK | DASHED));
            return *this;
        }

        /** Keeps only those attributes defined for the other cell as well. 
         */
        Font & andAttributesFrom(Font const & from) {
            font_ = font_ & (from.font_ & ~(BOLD | ITALIC | UNDERLINE | STRIKETHROUGH | BLINK | DASHED));
            return *this;
        }

    private:
        static constexpr uint16_t BOLD = 1 << 15;
        static constexpr uint16_t ITALIC = 1 << 14;
        static constexpr uint16_t UNDERLINE = 1 << 13;
        static constexpr uint16_t STRIKETHROUGH = 1 << 12;
        static constexpr uint16_t BLINK = 1 << 11;
        static constexpr uint16_t DOUBLE_WIDTH = 1 << 10;
        static constexpr uint16_t DASHED = 1 << 9;


        static constexpr uint16_t SIZE_MASK = 7;

        Font(uint16_t raw):
            font_{raw} {
        }

        uint16_t font_;

    }; // ui::Font

} // namespace ui
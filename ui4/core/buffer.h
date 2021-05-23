#pragma once

#include "helpers/helpers.h"
#include "geometry.h"

namespace ui {


    /** UI backing buffer, a 2D array of cells. 
     */
    class Buffer final {
    public:

        class Cell;

        explicit Buffer(Size const & size):
            size_{size} {
            create(size_);
        }

        Buffer(Buffer && from) noexcept:
            size_{from.size_},
            rows_{from.rows_} {
            from.size_ = Size{0,0};
            from.rows_ = nullptr;
        }

        Buffer & operator = (Buffer && from) noexcept {
            clear();
            size_ = from.size_;
            rows_ = from.rows_;
            from.size_ = Size{0,0};
            from.rows_ = nullptr;
            return *this;
        }

        ~Buffer() {
            clear();
        }

        Size const & size() const {
            return size_;
        }

        int width() const {
            return size_.width(); 
        }

        int height() const {
            return size_.height();
        }

        bool contains(Point const & x) const {
            return Rect{size_}.contains(x);
        }

        Cell const & operator [] (Point at) const;

        Cell & operator [] (Point at);

    private:

        void create(Size const & size);

        void clear();

        Size size_;

        Cell ** rows_ = nullptr;
    }; // ui::Buffer

    /** Single cell in a buffer. 
     
        - how to make special objects lock free? 
     
     */
    class Buffer::Cell {

    private:
        /** The unicode codepoint stored in the cell. 
         
            Since there is only 10ffff characters in unicode this leaves us with 11 bits of extra information that can be stored in a cell. 
         */
        char32_t codepoint_;
        /** Foreground color. 
         */
        //Color fg_;
        /** Background color. 
         */
        //Color bg_;
        /** Decorations color (underline, strikethrough, etc.)
         */
        //Color decor_;
        /** Font used for rendering.
         */
        //Font font_;
        /** Border to be displayed around the cell. 
         */
        //Border_;

    }; // ui::Buffer::Cell

    inline Buffer::Cell const & Buffer::operator [] (Point at) const {
        ASSERT(contains(at));
        return rows_[at.y()][at.x()];
    }

    inline Buffer::Cell & Buffer::operator [] (Point at) {
        ASSERT(contains(at));
        return rows_[at.y()][at.x()];
    }


} // namespace ui
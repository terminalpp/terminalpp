#pragma once

#include "helpers/char.h"

#include "common.h"
#include "geometry.h"

namespace ui2 {


    /** Single cell of the buffer. 
     
        The cell represents the codepoint to be displayed and its graphic properties. 
     */
    class Cell { 
    public:

        // TODO the default constructor should really be different!
        Cell():
            codepoint_{' '},
            fg_{Color::White},
            bg_{Color::Black},
            decor_{Color::Black} {
        }

        /** \name Codepoint
          
            The unencoded Unicode codepoint to be displayed in the cell.

            Since the codepoint is stored in a 32bit character and unicode only supports up to 0x10ffff codepoints, there are 11 unused bits. These are masked by the codepoint getter and setter so that they can be used by the buffers for extra information. 
         */
        //@{
        char32_t const & codepoint() const {
            return codepoint_ & 0x1fffff;
        }

        Cell & setCodepoint(char32_t value) {
            codepoint_ = (codepoint_ & 0xffe00000) + (value & 0x1fffff);
            return *this;
        }
        //@}

        /** \name Foreground Color
         
            The color of the character displayed in the cell.
         */
        //@{
        Color fg() const {
            return fg_;
        }

        Cell & setFg(Color value) {
            fg_ = value;
            return *this;
        }
        //@}

        /** \name Background Color
         
            The background color of the cell. 
         */
        //@{
        Color bg() const {
            return bg_;
        }

        Cell & setBg(Color value) {
            bg_ = value;
            return *this;
        }
        //@}

        /** \name Decoration Color. 
         
            The color of font decorations, such as underline or strikethrough. 
         */
        //@{
        Color decor() const {
            return decor_;
        }

        Cell & setDecor(Color value) {
            decor_ = value;
            return *this;
        }
        //@}

        /** \name Font
         
            The font used to render the cell. 
         */
        //@{
        Font font() const {
            return font_;
        }

        Cell & setFont(Font value) {
            font_ = value;
            return *this;
        }
        //@}

        /** \name Border
         
            The border of the cell. 
         */
        //@{
        Border border() const {
            return border_;
        }

        Cell & setBorder(Border const & value) {
            border_ = value;
            return *this;
        }
        //@}

    private:

        friend class Buffer;

        char32_t codepoint_;

        Color fg_;
        Color bg_;
        Color decor_;

        Font font_;

        Border border_;

    }; // ui::Cell

    /** The UI backing buffer. 
     
        The buffer contains a 2D array of cells describing the physical screen and allows their basic access. 

        For performance reasons, the 2D array is organized on per row basis so that scrolling rows is a simple pointer swap and does not have to involve any complex memory copying, which in case of fast scrolling terminal commands can be quite expensive. 
     */
    class Buffer {
    public:

        Buffer(int width, int height):
            width_{width},
            height_{height} {
            create(width, height);
        }

        Buffer(Buffer && from):
            width_{from.width_},
            height_{from.height_},
            rows_{from.rows_} {
            from.width_ = 0;
            from.height_ = 0;
            from.rows_ = nullptr;
        }

        Buffer & operator = (Buffer && from) {
            clear();
            width_ = from.width_;
            height_ = from.height_;
            rows_ = from.rows_;
            from.width_ = 0;
            from.height_ = 0;
            from.rows_ = nullptr;
            return *this;
        }

        ~Buffer() {
            clear();
        }

        int width() const {
            return width_;
        }

        int height() const {
            return height_;
        }

        Cell const & at(int x, int y) const {
            ASSERT(x >= 0 && x < width_);
            ASSERT(y >= 0 && y < height_);
            return rows_[y][x];
        }

        Cell const & at(Point p) const {
            return at(p.x(), p.y());
        }

        Cell & at(int x, int y) {
            ASSERT(x >= 0 && x < width_);
            ASSERT(y >= 0 && y < height_);
            return rows_[y][x];
        }

        Cell & at(Point p) {
            return at(p.x(), p.y());
        }

        void swapRows(int first, int second) {
            ASSERT(first >= 0 && first < height_);
            ASSERT(second>= 0 && second < height_);
            std::swap(rows_[first], rows_[second]);
        }

        /** Resizes the buffer. 
         
            Backing buffer resize is destructive operation and after a resize the whole contents has to be repainted. 
         */
        void resize(int width, int height) {
            clear();
            create(width, height);
        }

    protected:

        void create(int width, int height) {
            rows_ = new Cell*[height];
            for (int i = 0; i < height; ++i)
                rows_[i] = new Cell[width];
            width_ = width;
            height_ = height;
        }

        void clear() {
            for (int i = 0; i < height_; ++i)
                delete rows_[i];
            delete rows_;
        }

        int width_;
        int height_;
        Cell ** rows_;

        /** Returns the value of the unused bits in the given cell's codepoint so that the buffer can store extra information for each cell. 
         */
        static char32_t GetUnusedBytes(Cell const & cell) {
            return cell.codepoint_ & 0xffe00000;
        }

        /** Sets the unused bytes value for the given cell to store extra information by the buffer. 
         */
        static void SetUnusedBytes(Cell & cell, char32_t value) {
            cell.codepoint_ = (cell.codepoint_ & 0x1fffff) + (value & 0xffe00000);
        }

    };

} // namespace ui2
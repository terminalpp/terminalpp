#pragma once

#include "helpers/char.h"

#include "common.h"
#include "geometry.h"

namespace ui {


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

        Cell & operator = (Cell const & other) {
            if (this != &other)
                memcpy(this, &other, sizeof(Cell));
            return *this;
        }

        /** \name Codepoint
          
            The unencoded Unicode codepoint to be displayed in the cell.

            Since the codepoint is stored in a 32bit character and unicode only supports up to 0x10ffff codepoints, there are 11 unused bits. These are masked by the codepoint getter and setter so that they can be used by the buffers for extra information. 
         */
        //@{
        char32_t codepoint() const {
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

    /** Basic properties for an active cursor. 
     */
    class Cursor {
    public:

        Cursor():
            codepoint_{0x2581},
            visible_{true},
            blink_{true},
            color_{Color::White} {
        }
           
        char32_t codepoint() const {
            return codepoint_;
        }

        bool visible() const {
            return visible_;
        }

        bool blink() const {
            return blink_;
        }

        Color color() const {
            return color_;
        }

        Cursor & setCodepoint(char32_t value) {
            codepoint_ = value;
            return *this;
        }

        Cursor & setVisible(bool value = true) {
            visible_ = value;
            return *this;
        }

        Cursor & setBlink(bool value = true) {
            blink_ = value;
            return *this;
        }

        Cursor & setColor(Color value) {
            color_ = value;
            return *this;
        }

    private:
        char32_t codepoint_;
        bool visible_;
        bool blink_;
        Color color_;
    }; // ui::Cursor

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

        /** Returns the cursor to be displayed. 
         
            If the cursor position has been invalidated in the meanting, returns the storec cursor with visibility set to false. 
         */
        Cursor cursor() const {
            if (Rect::FromWH(width_, height_).contains(cursorPosition_) && GetUnusedBits(at(cursorPosition_)) & CURSOR_POSITION)
                return cursor_;
            else
                return Cursor{cursor_}.setVisible(false);
        }

        /** Sets the cursor to be displayed at the stored position. 
         */
        void setCursor(Cursor const & value) {
            cursor_ = value;
        }

        /** Returns the cursor position. 
         */
        Point cursorPosition() const {
            return cursorPosition_;
        }

        /** Sets the cursor position. 
         
            Marks the cell as containing the cursor. If the cell is overwritten in the future, the flag is cleared and the cursor visibility will be disabled. 

            Cursor position can also be set to out of bounds coordinates, which also hides the cursor. 
         */
        void setCursorPosition(Point value) {
            cursorPosition_ = value;
            if (Rect::FromWH(width_, height_).contains(value))
                SetUnusedBits(at(value), CURSOR_POSITION);
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
            SetUnusedBits(rows_[y][x], 0);
            return rows_[y][x];
        }

        Cell & at(Point p) {
            return at(p.x(), p.y());
        }

        /** Resizes the buffer. 
         
            Backing buffer resize is destructive operation and after a resize the whole contents has to be repainted. 
         */
        void resize(int width, int height) {
            clear();
            create(width, height);
        }

    protected:

        friend class Canvas;

        void create(int width, int height) {
            rows_ = new Cell*[height];
            for (int i = 0; i < height; ++i)
                rows_[i] = new Cell[width];
            width_ = width;
            height_ = height;
        }

        void clear() {
            // rows can be nullptr if they have been backed up by a swap when resizing
            if (rows_ != nullptr) {
                for (int i = 0; i < height_; ++i)
                    delete [] rows_[i];
                delete [] rows_;
            }
            height_ = 0;
            width_ = 0;
        }

        int width_;
        int height_;
        Cell ** rows_;

        /** Cursor properties.  */
        Cursor cursor_;
        /** Cursor position. */
        Point cursorPosition_;

        /** Returns the value of the unused bits in the given cell's codepoint so that the buffer can store extra information for each cell. 
         */
        static char32_t GetUnusedBits(Cell const & cell) {
            return cell.codepoint_ & 0xffe00000;
        }

        /** Sets the unused bytes value for the given cell to store extra information by the buffer. 
         */
        static void SetUnusedBits(Cell & cell, char32_t value) {
            cell.codepoint_ = (cell.codepoint_ & 0x1fffff) + (value & 0xffe00000);
        }

        /** Unused bits flag that confirms that the cell has a visible cursor in it. 
         */
        static char32_t constexpr CURSOR_POSITION = 0x200000;

    };

} // namespace ui
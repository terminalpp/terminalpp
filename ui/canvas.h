#pragma once

#include "font.h"
#include "color.h"
#include "border.h"
#include "geometry.h"

namespace ui {

    class Widget;
    class Renderer;

    class Canvas {
        friend class Widget;
        friend class Renderer;
    public:

        class Cursor;
        class Cell;
        class Buffer;

        Canvas(Buffer & buffer);

        Rect rect() const {
            return Rect{size()};
        }

        Rect visibleRect() const {
            return visibleArea_.rect();
        }

        Size size() const {
            return size_;
        }

        int width() const {
            return size_.width();
        }

        int height() const {
            return size_.height();
        }

        /** \name Text metrics
         */
        //@{

        /** Information about a single line of text. 
         */
        struct TextLine {
            /** Width of the line in cells for single-width font of size 1. 
             */
            int width; 
            /** The actual number of codepoints in the line.
             */
            int chars;
            /** First character of the line.
             */
            Char::iterator_utf8 begin;
            /** End of the line (exclusive). 
             */
            Char::iterator_utf8 end;
        }; // Canvas::TextLine

        static constexpr int NoWordWrap = -1;

        static std::vector<TextLine> GetTextMetrics(std::string const & text, int wordWrapAt = NoWordWrap);

        static TextLine GetTextLine(Char::iterator_utf8 & begin, Char::iterator_utf8 const & end, int wordWrapAt = NoWordWrap);

        //@}

        /** \name State
         */
        //@{

        Color fg() const {
            return fg_;
        }

        void setFg(Color value) {
            fg_ = value;
        }

        Color bg() const {
            return bg_;
        }

        void setBg(Color value) {
            bg_ = value;
        }

        Font font() const {
            return font_;
        }

        void setFont(Font value) {
            font_ = value;
        }

        //@}

        /** \name Drawing
         */
        //@{

        /** Draws the buffer starting from given top left corner. 
         */
        Canvas & drawBuffer(Buffer const & buffer, Point at);

        Canvas & fill(Rect const & rect) {
            return fill(rect, bg_);
        }

        Canvas & fill(Rect const & rect, Color color);

        /** Fills the given rectangle with the specified cell. 
         
            Overrides any previous information, even if the cell would be transparent. 
         */
        Canvas & fill(Rect const & rect, Cell const & fill);

        Canvas & textOut(Point x, std::string const & str) {
            return textOut(x, Char::BeginOf(str), Char::EndOf(str));
        }

        Canvas & textOut(Point x, Char::iterator_utf8 begin, Char::iterator_utf8 end);



        Canvas & border(Border const & border, Point from, Point to);
        Canvas & border(Border const & border, Rect const & rect);


        Canvas & verticalScrollbar(int size, int offset);
        Canvas & horizontalScrollbar(int size, int offset);

        //@}

        /** \name Single cell access. 
         */
        //@{

        Cell & at(Point const & coords);
        Cell const & at(Point const & coords) const;

        //@}

        /** \name Helpers
         
            Don't really know where to put these...
         */
        //@{

        static std::pair<int, int> ScrollBarDimensions(int length, int max, int offset) {
            int sliderSize = std::max(1, length * length / max);
            int sliderStart = (offset + length == max) ? (length - sliderSize) : (offset * length / max);
            // make sure that slider starts at the top only if we are really at the top
            if (sliderStart == 0 && offset != 0)
                sliderStart = 1;
            // if the slider would go beyond the length, adjust the slider start
            if (sliderStart + sliderSize > length)
                sliderStart = length - sliderSize;
            return std::make_pair(sliderStart, sliderStart + sliderSize);
        }	
        //@}



    private:


        Color fg_;
        Color bg_;
        Color decor_;
        Font font_;

    protected:

        /** Visible area of the canvas. 
         
            Each widget remembers its visible area, which consists of the pointer to its renderer, the offset of the widget's top-left corner in the renderer's absolute coordinates and the area of the widget that translates to a portion of the renderer's buffer. 
        */
        class VisibleArea {
        public:


            VisibleArea() = default;

            VisibleArea(VisibleArea const & ) = default;

            VisibleArea(Point const & offset, Rect const & rect):
                offset_{offset},
                rect_{rect} {
            }

            /** The offset of the canvas' coordinates from the buffer ones, 
             
                Corresponds to the buffer coordinates of canvas' [0,0].
             */
            Point offset() const {
                return offset_;
            }

            /** The rectangle within the canvas that is backed by the buffer, in the canvas' coordinates. 
             */
            Rect const & rect() const {
                return rect_;
            }

            /** The visible are in buffer coordinates. 
             */
            Rect bufferRect() const {
                return rect_ + offset_;
            }

            VisibleArea clip(Rect const & rect) const {
                return VisibleArea{offset_ + rect.topLeft(), (rect_ & rect) - rect.topLeft()};
            }

            VisibleArea offset(Point const & by) const {
                return VisibleArea{offset_ - by, rect_ + by};
            }

        private:

            Point offset_;
            Rect rect_;
        }; // ui::Canvas::VisibleArea

        /** Creates canvas for given widget. 
         */
        Canvas(Buffer & buffer, VisibleArea const & visibleArea, Size const & size);

    private:

        VisibleArea visibleArea_;
        Buffer * buffer_;
        Size size_;

    }; // ui::Canvas

    class Canvas::Cursor {
    public:

        char32_t const & codepoint() const {
            return codepoint_;
        }

        bool const & visible() const {
            return visible_;
        }

        bool const & blink() const {
            return blink_;
        }

        Color const & color() const {
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
    }; // ui::Canvas::Cursor

    class Canvas::Cell {
        friend class Canvas::Buffer;
    public:

        /** Default constructor.
         */
        Cell():
            codepoint_{' '},
            fg_{Color::White},
            bg_{Color::Black},
            decor_{Color::White},
            font_{},
            border_{} {
        }

        /** \name Codepoint of the cell. 
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

        /** \name Foreground (text) color. 
         */
        //@{
        Color const & fg() const {
            return fg_;
        }

        Cell & setFg(Color value) {
            fg_ = value;
            return *this;
        }
        //@}

        /** \name Background (fill) color. 
         */
        //@{
        Color const & bg() const {
            return bg_;
        }

        Cell & setBg(Color value) {
            bg_ = value;
            return *this;
        }

        //@}

        /** \name Decoration (underline, strikethrough) color. 
         */
        //@{
        Color const & decor() const {
            return decor_;
        }

        Cell & setDecor(Color value) {
            decor_ = value;
            return *this;
        }
        //@}

        /** \name Font. 
         */
        //@{

        Font const & font() const {
            return font_;
        }

        Cell & setFont(Font value) {
            font_ = value;
            return *this;
        }
        //@}

        /** \name Border. 
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

        Cell & operator = (Cell const & other) = default;

    private:

        char32_t codepoint_;
        Color fg_;
        Color bg_;
        Color decor_;
        Font font_;
        Border border_;
    }; // ui::Canvas::Cell


    class Canvas::Buffer {
    public:

        Buffer(Size const & size):
            size_{size} {
            create(size);
        }

        Buffer(Buffer && from):
            size_{from.size_},
            rows_{from.rows_} {
            from.size_ = Size{0,0};
            from.rows_ = nullptr;
        }

        Buffer & operator = (Buffer && from) {
            clear();
            size_ = from.size_;
            rows_ = from.rows_;
            from.size_ = Size{0,0};
            from.rows_ = nullptr;
            return *this;
        }          

        virtual ~Buffer() {
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

        /** Determines whether given point lies within the buffer's area. 
         */
        bool contains(Point const & x) const {
            return Rect{size_}.contains(x);
        }

        void resize(Size const & value) {
            if (size_ == value)
                return;
            clear();
            create(value);
        }

        Cell const & at(int x, int y) const {
            return at(Point{x, y});
        }

        Cell const & at(Point p) const {
            return cellAt(p);
        }

        Cell & at(int x, int y) {
            return at(Point{x, y});
        }

        Cell & at(Point p) {
            Cell & result = cellAt(p);
            // clear the unused bits because of non-const access
            SetUnusedBits(result, 0);
            return result;
        }

        /** Returns the cursor properties. 
         */
        Cursor cursor() const {
            return cursor_;
        }

        Point cursorPosition() const {
            if (contains(cursorPosition_) && (GetUnusedBits(at(cursorPosition_)) & CURSOR_POSITION) == 0)
                return Point{-1,-1};
            else 
                return cursorPosition_;
        }

        /** Sets the cursor and position. 
         */
        void setCursor(Cursor const & value, Point position) {
            cursor_ = value;
            cursorPosition_ = position;
            if (contains(cursorPosition_))
                SetUnusedBits(at(cursorPosition_), CURSOR_POSITION);
        }

        /** Fills portion of given row with the specified cell. 
         
            Exponentially increases the size of copied cells for performance.
         */
        void fillRow(int row, Cell const & fill, int from, int cols) {
            Cell * r = rows_[row];
            r[from] = fill;
            size_t i = 1;
            size_t next = 2;
            while (next < cols) {
                memcpy(r + from + i, r + from, sizeof(Cell) * i);
                i = next;
                next *= 2;
            }
            memcpy(r + from + i, r + from, sizeof(Cell) * (cols - i));
        }

    protected:

        /*
        Cell ** rows() {
            return rows_;
        }
        */

        Cell const & cellAt(Point const & p) const {
            ASSERT(Rect{size_}.contains(p));
            return rows_[p.y()][p.x()];
        }

        Cell & cellAt(Point const & p) {
            ASSERT(Rect{size_}.contains(p));
            return rows_[p.y()][p.x()];
        }

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

    protected:

        void create(Size const & size) {
            rows_ = new Cell*[size.height()];
            for (int i = 0; i < size.height(); ++i)
                rows_[i] = new Cell[size.width()];
            size_ = size;
        }

        void clear() {
            // rows can be nullptr if they have been backed up by a swap when resizing
            if (rows_ != nullptr) {
                for (int i = 0; i < size_.height(); ++i)
                    delete [] rows_[i];
                delete [] rows_;
            }
            size_ = Size{0,0};
        }

        Size size_;
        Cell ** rows_;

        Cursor cursor_;
        Point cursorPosition_;

    }; // ui::Canvas::Buffer

    inline Canvas::Canvas(Canvas::Buffer & buffer):
        Canvas(buffer, VisibleArea{Point{0,0}, buffer.size()}, buffer.size()) {
    }

    inline Canvas::Cell & Canvas::at(Point const & coords) {
        return buffer_->at(coords + visibleArea_.offset());
    }

    inline Canvas::Cell const & Canvas::at(Point const & coords) const {
        return buffer_->at(coords + visibleArea_.offset());
    }

} // namespace ui
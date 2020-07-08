#pragma once

#include "font.h"
#include "color.h"
#include "geometry.h"

namespace ui3 {

    class Widget;
    class Renderer;

    class Canvas {
        friend class Widget;
        friend class Renderer;
    public:

        class Cursor;
        class Cell;
        class Buffer;

        Rect rect() const {
            return Rect{size()};
        }

        Size size() const {
            return size_;
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

        Canvas & fill(Rect const & rect) {
            return fill(rect, bg_);
        }
        Canvas & fill(Rect const & rect, Color color);

        Canvas & textOut(Point x, std::string const & str) {
            return textOut(x, Char::BeginOf(str), Char::EndOf(str));
        }

        Canvas & textOut(Point x, Char::iterator_utf8 begin, Char::iterator_utf8 end);

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
            friend class Renderer;
        public:

            VisibleArea():
                renderer_{nullptr} {
            }

            VisibleArea(VisibleArea const & ) = default;

            Renderer * renderer() const {
                return renderer_;
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

            bool attached() const {
                return renderer_ != nullptr;
            }

            void attach(Renderer * renderer) {
                renderer_ = renderer;
                rect_ = Rect{};
                offset_ = Point{};
            }

            /** Detaches the visible area from the renderer, thus invalidating it. 
             */
            void detach() {
                renderer_ = nullptr;
            }

            VisibleArea clip(Rect const & rect) const {
                return VisibleArea{renderer_, offset_ + rect.topLeft(), (rect_ & rect) - rect.topLeft()};
            }

            VisibleArea offset(Point const & by) const {
                return VisibleArea{renderer_, offset_ - by, rect_};
            }

        private:

            VisibleArea(Renderer * renderer, Point const & offset, Rect const & rect):
                renderer_{renderer}, 
                offset_{offset},
                rect_{rect} {
            }

            Renderer * renderer_;
            Point offset_;
            Rect rect_;
        }; // ui::Canvas::VisibleArea

        /** Creates canvas for given widget. 
         */
        Canvas(VisibleArea const & visibleArea, Size const & size);

    private:


        VisibleArea visibleArea_;
        Buffer & buffer_;
        Size size_;

    }; // ui::Canvas

    class Canvas::Cursor {
    public:

    private:
        char32_t codepoint_;
        bool visible_;
        bool blink_;
        Color color_;
    }; // ui::Canvas::Cursor

    class Canvas::Cell {
        friend class Canvas::Buffer;
    public:
        class CodepointSetter {
            friend class Cell;
        public:
            CodepointSetter(CodepointSetter &&) = default;
            CodepointSetter(CodepointSetter const &) = delete;

            CodepointSetter & operator = (char32_t value) {
                codepoint_ = (codepoint_ & 0xffe00000) + (value & 0x1fffff);
                return *this;
            }

            operator char32_t () {
                return codepoint_ & 0x1fffff;
            }

        private:
            CodepointSetter(char32_t & codepoint):
                codepoint_{codepoint} {
            }
            char32_t & codepoint_;
        };  // Cell::CodepointSetter

        /** Default constructor.
         */
        Cell():
            codepoint_{' '},
            fg_{Color::White},
            bg_{Color::Black},
            decor_{Color::White},
            font_{} {
        }

        /** \name Codepoint of the cell. 
         */
        //@{
        char32_t codepoint() const {
            return codepoint_ & 0x1fffff;
        }

        CodepointSetter codepoint() {
            return CodepointSetter{codepoint_};
        }
        //@}

        /** \name Foreground (text) color. 
         */
        //@{
        Color const & fg() const {
            return fg_;
        }

        Color & fg() {
            return fg_;
        }
        //@}

        /** \name Background (fill) color. 
         */
        //@{
        Color const & bg() const {
            return bg_;
        }

        Color & bg() {
            return bg_;
        }
        //@}

        /** \name Decoration (underline, strikethrough) color. 
         */
        //@{
        Color const & decor() const {
            return decor_;
        }

        Color & decor() {
            return decor_;
        }
        //@}

        /** \name Font. 
         */
        //@{

        Font const & font() const {
            return font_;
        }

        Font & font() {
            return font_;
        }
        //@}
    private:

        char32_t codepoint_;
        Color fg_;
        Color bg_;
        Color decor_;
        Font font_;
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

    protected:

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

    private:

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

    }; // ui::Renderer::Buffer


} // namespace ui
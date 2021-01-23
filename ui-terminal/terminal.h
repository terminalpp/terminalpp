#pragma once

#include "helpers/locks.h"

#include "tpp-lib/pty.h"
#include "tpp-lib/pty_buffer.h"


#include "ui/widget.h"

namespace ui {

    /** Base terminal specification. 
     
        Provides minimal interface of a terminal. On top of being a widget, provides the events for adding new history line and changing the active buffer. 
     */
    class Terminal : public virtual Widget, public tpp::PTYBuffer<tpp::PTYMaster> {
    public:
        using Cell = Canvas::Cell;
        using Cursor = Canvas::Cursor;

        /** Palette
         */
        class Palette {
        public:
            static Palette Colors16();
            static Palette XTerm256(); 

            Palette():
                defaultFg_{Color::White},
                defaultBg_{Color::Black},
                colors_{{Color::Black, Color::White}} {
            }

            Palette(size_t size, Color defaultFg = Color::White, Color defaultBg = Color::Black):
                defaultFg_(defaultFg),
                defaultBg_(defaultBg) {
                colors_.resize(size);
            }

            Palette(std::initializer_list<Color> colors, Color defaultFg = Color::White, Color defaultBg = Color::Black);

            Palette(Palette const & from) = default;

            Palette(Palette && from) noexcept = default;


            Palette & operator = (Palette const & other) = default;
            Palette & operator = (Palette && other) noexcept = default;

            size_t size() const {
                return colors_.size();
            }

            Color defaultForeground() const {
                return defaultFg_;
            }

            Color defaultBackground() const {
                return defaultBg_;
            }

            void setDefaultForeground(size_t index) {
                defaultFg_ = colors_[index];
            }

            void setDefaultForeground(Color color) {
                defaultFg_ = color;
            }

            void setDefaultBackground(size_t index) {
                defaultBg_ = colors_[index];
            }

            void setDefaultBackground(Color color) {
                defaultBg_ = color;
            }

            Color operator [] (size_t index) const {
                ASSERT(index < size());
                return colors_[index];
            } 

            Color & operator [] (size_t index) {
                ASSERT(index < size());
                return colors_[index];
            } 

        private:

            Color defaultFg_;
            Color defaultBg_;
            std::vector<Color> colors_;

        }; // ui::Terminal::Palette

        enum class BufferKind {
            Normal,
            Alternate,
        };

        using BufferChangeEvent = Event<BufferKind>;

        /** The terminal buffer is a canvas buffer augmented with terminal specific properties such as the scroll window. 
         
            The buffer is tightly coupled with its terminal. 
         */
        class Buffer : public Canvas::Buffer {
            friend class Terminal;
        public:
            Buffer(BufferKind bufferKind, Terminal * terminal, Cell const & defaultCell):
                Canvas::Buffer{MinSize(terminal->size())},
                bufferKind_{bufferKind},
                terminal_{terminal},
                currentCell_{defaultCell},
                scrollEnd_{height()} {
                fill(currentCell_);
            }

            BufferKind kind() const {
                return bufferKind_;
            }

            void reset(Color fg, Color bg) {
                currentCell_ = Cell{}.setFg(fg).setDecor(fg).setBg(bg);
                scrollStart_ = 0;
                scrollEnd_ = height();
                inverseMode_ = false;
                setCursorPosition(Point{0,0});
                fill(currentCell_);
            }

            Cell const & currentCell() const {
                return currentCell_;
            }

            Cell & currentCell() {
                return currentCell_;
            }

            int scrollStart() const {
                return scrollStart_;
            }

            int scrollEnd() const {
                return scrollEnd_;
            }

            void setScrollRegion(int start, int end) {
                scrollStart_ = start;
                scrollEnd_ = end;
            }

            /** Resizes the buffer and updates the state accordingly. 
             
                When called, the terminal must already be resized. 
             */
            void resize();

            /** Writes given character at the cursor position using the current cell attributes and returns the cell itself. 
             */
            Cell & addCharacter(char32_t codepoint);

            /** Adds new line. 
             
                Does not add any visible character, but marks the last character position, if valid as line end and updates the cursor position accordingly. 
             */
            void newLine();

            void carriageReturn() {
                cursorPosition_.setX(0);
            }

            /** Inserts N characters right of the position, shifting the rest of the line appropriately to the right. 
             
                Characters that would end up right of the end of the buffer are discarded. 
             */
            void insertCharacters(Point from, int num) {
                ASSERT(from.x() + num <= width() && num >= 0);
                // first copy the characters
                for (int c = width() - 1, e = from.x() + num; c >= e; --c)
                    at(c, from.y()) = at(c - num, from.y());
                for (int c = from.x(), e = from.x() + num; c < e; ++c)
                    at(c, from.y()) = currentCell_;
            }

            /** Deletes N characters right of the position, shifts the line to the left accordingly and fills the rightmost cells with given current cell. 
             */
            void deleteCharacters(Point from, int num) {
                for (int c = from.x(), e = width() - num; c < e; ++c) 
                    at(c, from.y()) = at(c + num, from.y());
                for (int c = width() - num, e = width(); c < e; ++c)
                    at(c, from.y()) = currentCell_;
            }

            void insertLine(int top, int bottom, Cell const & fill);

            /** TODO this can be made faster and the lines can be moved only once. 
             */
            void insertLines(int lines, int top, int bottom, Cell const & fill) {
                while (lines-- > 0)
                    insertLine(top, bottom, fill);
            }

            /** Deletes given line and scrolls all lines in the given range (top-bottom) one line up, filling the new line at the bottom with the given cell. 
             
                Triggers the terminal's onNewHistoryLine event. 
             */
            void deleteLine(int top, int bottom, Cell const & fill);

            void deleteLines(int lines, int top, int bottom, Cell const & fill) {
                while (lines-- > 0)
                    deleteLine(top, bottom, fill);
            }

            /** Adjusts the cursor position to remain within the buffer and shifts the buffer if necessary. 
             
                Makes sure that cursor position is within the buffer bounds. If the cursor is too far left, it is reset on next line, first column. If the cursor is below the buffer the buffer is scrolled appropriately. 

                Returns the cursor position after the adjustment, which is guaranteed to be always valid and within the buffer bounds.
            */
            Point adjustedCursorPosition();

            /** Overrides canvas cursor position to disable the check whether the cell has the cursor flag. 
             
                The cursor in terminal is only one and always valid at the coordinates specified in the buffer. 
                
                Note that the cursor position may *not* be valid within the buffer (cursor after text can be advanced beyond the buffer itself). To obtain a valid cursor position, which may shift the buffer contents, use the adjustCursorPosition function instead. 
            */
            Point cursorPosition() const {
                return cursorPosition_;
            }

            void setCursorPosition(Point pos) {
                cursorPosition_ = pos;
                invalidateLastCharacter();
            }

            // TODO advance cursor that is faster that setCursorPosition

            void setCursor(Canvas::Cursor const & value, Point position) {
                cursor_ = value;
                setCursorPosition(position);
            }

            void saveCursor() {
                cursorStack_.push_back(cursorPosition());
            }

            void restoreCursor() {
                if (cursorStack_.empty())
                    return;
                Point pos = cursorStack_.back();
                ASSERT(pos.x() >= 0 && pos.y() >= 0);
                cursorStack_.pop_back();
                if (pos.x() >= width())
                    pos.setX(width() - 1);
                if (pos.y() >= height())
                    pos.setX(height() - 1);
                setCursorPosition(pos);
            }

            void resetAttributes();

            bool isBold() const {
                return bold_;
            }

            /** Sets the bold typeface of the current cell's font. 
             
                If displayBold is false then the the buffer remembers that current font face is bold, but actually won't change the display attributes. This way bold font can be rendered in bright colors (bold is bright setting) and bold fonts can be disabled at the same time. 
             */
            void setBold(bool value, bool displayBold) {
                bold_ = value;
                if (bold_ && displayBold)
                    currentCell_.font().setBold(value);
            }

            void setInverseMode(bool value) {
                if (inverseMode_ == value)
                    return;
                inverseMode_ = value;
                Color fg = currentCell_.fg();
                Color bg = currentCell_.bg();
                currentCell_.setFg(bg).setDecor(bg).setBg(fg);
            }

            Canvas canvas() {
                return Canvas{*this};
            }

            static bool IsLineEnd(Cell const & c) {
                return GetUnusedBits(c) & END_OF_LINE;
            }

        private:

            /** Returns the start of the line that contains the cursor including any word wrap. 
             
                I.e. if the cursor is on line that started 3 lines above and was word-wrapped to the width of the terminal returns the current cursor row minus three. 
            */
            int getCursorRowWrappedStart() const;

            /** Returns true if the given line contains only whitespace characters from given column to its width. 
             
                If the column start is greater or equal to the width, returns true. 
            */
            bool hasOnlyWhitespace(Cell * row, int from, int width);

            void markAsLineEnd(Point p) {
                if (p.x() >= 0)
                    SetUnusedBits(at(p), END_OF_LINE);
            }

            /** Invalidates the cached location of last printable character for end of line detection in the terminal. 
             
                Typically occurs during forceful cursor position updates, etc. 
             */
            void invalidateLastCharacter() {
                lastCharacter_ = Point{-1, -1};
            }

            /** Prevents terminal buffer of size 0 to be instantiated. 
             
                TODO should this move to Canvas::Buffer? 
             */
            static Size MinSize(Size request) {
                if (request.width() <= 0)
                    request.setWidth(1);
                if (request.height() <= 0)
                    request.setHeight(1);
                return request;
            }

            BufferKind bufferKind_;
            Terminal * terminal_;

            Cell currentCell_;
            int scrollStart_ = 0;
            int scrollEnd_;
            bool inverseMode_ = false;

            /** When bold is bright *and* bold is not displayed this is the only way to determine whether to use bright colors because the cell's font is not bold. 
             */
            bool bold_ = false;

            Point lastCharacter_ = Point{-1, -1};
            std::vector<Point> cursorStack_;

            /** Flag designating the end of line in the buffer. 
             */
            static constexpr char32_t END_OF_LINE = Cell::FIRST_UNUSED_BIT;

        }; // ui::Terminal::Buffer


        struct HistoryRow {
        public:
            BufferKind buffer;
            int width;
            Cell const * cells;
        };

        using NewHistoryRowEvent = Event<HistoryRow>;

        /** Triggered when the terminal enables, or disables an alternate mode. 
         
            Can be called from any thread, expects the terminal buffer lock.
         */
        BufferChangeEvent onBufferChange;

        /** Triggered when new row is evicted from the terminal's scroll region. 
         
            Can be triggered from any thread, expects the terminal buffer lock. The cells will be reused by the terminal after the call returns. 
         */
        NewHistoryRowEvent onNewHistoryRow;

        Palette const & palette() const {
            return palette_;
        }

        virtual void setPalette(Palette && value) {
            if (& palette_ != & value) {
                palette_ = std::move(value);
                setBackground(palette_.defaultBackground());
            }
        }

        /** Returns the priority lock of the terminal's buffer so that it can be locked by 3rd parties for updates.
         */
        PriorityLock & lock() {
            return lock_;
        }

    protected:
        /** Returns the default empty cell of the terminal. 
         */
        Cell defaultCell() const {
            return Cell{}.setFg(palette_.defaultForeground()).setDecor(palette_.defaultForeground()).setBg(palette_.defaultBackground());
        }

        Terminal(tpp::PTYMaster * pty, Palette && palette):
            PTYBuffer{pty},
            palette_{std::move(palette)} {
            setBackground(palette_.defaultBackground());
        }

        void paint(Canvas & canvas) override {
            {
                std::lock_guard<PriorityLock> g(lock_.priorityLock(), std::adopt_lock);
                paintLocked(canvas);
            }
            Widget::paint(canvas);
        }

        virtual void paintLocked(Canvas & canvas) = 0;

        /** Called when new row is evicted from the terminal and can be added to the history. 
         */
        virtual void newHistoryRow(NewHistoryRowEvent::Payload & row) {
            if (row.active()) {
                onNewHistoryRow(row, this);
            }
        }

        mutable PriorityLock lock_;

    private:

        Palette palette_;

    }; // ui::Terminal


    inline void Terminal::Buffer::resetAttributes() {
        currentCell_ = terminal_->defaultCell();
        bold_ = false;
        inverseMode_ = false;
    }



} // namespace ui
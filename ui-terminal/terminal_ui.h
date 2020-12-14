#pragma once
#include <algorithm>

#include "helpers/helpers.h"

#include "terminal.h"

namespace ui {

    /** Displays terminal history. 
     
     */
    class TerminalHistory : public virtual Widget {
    public:

        TerminalHistory(Terminal * terminal):
            terminal_{terminal} {
            setHeightHint(SizeHint::AutoSize());
            ASSERT(! terminal->onNewHistoryRow.attached());
            terminal->onNewHistoryRow.setHandler(& TerminalHistory::addHistoryRow, this);
        }

        int rows() const {
            return static_cast<int>(rows_.size());
        }

        int maxRows() const {
            return maxRows_;
        }

        void setMaxRows(int value) {
            if (value != maxRows_) {
                maxRows_ = std::max(value, 0);
                // TODO lock
                while (rows_.size() > static_cast<size_t>(maxRows_)) {
                    rows_.pop_front();
                }
            }
        }

    protected:
        /** Called when new line should be added to the history. 
         
            The terminal buffer must be locked when the function is called. 
         */
        void addHistoryRow(Terminal::NewHistoryRowEvent::Payload & e);

        void addHistoryRow(int width, Terminal::Cell * cells) {
            rows_.emplace_back(width, cells);
            if (rows_.size() > maxRows_)
                rows_.pop_front();
        }

    public:

        void resize(Size const & size) override;

    protected:

        /** Paints the history. 
         */
        void paint(Canvas & canvas) override;


    private:
        /** Single history row. 
         
            Contains the valid cells of the history row and their width. Contrary to the actual terminal buffer, the history only keeps valid cells, i.e. trims the row from right. 
         */
        struct Row {
            int width;
            Canvas::Cell * cells;

            ~Row() {
                delete [] cells;
            }

            Row(int width, Canvas::Cell * cells):
                width{width},
                cells{cells} {
            }

            Row(Row const &) = delete;

        }; // ui::TerminalHistory::Row

        Size getAutosizeHint() override {
            Size result = rect().size();
            result.setHeight(static_cast<int>(rows_.size()));
            return result;
        }

        /** The terminal whose history we display. 
         */
        Terminal const * terminal_;

        int maxRows_ = 0;
        std::deque<Row> rows_;

    }; // ui::TerminalHistory

    /** Terminal widget with user interface support. 
     
        Contains terminal as a child and provides history and UI features, such as scrollbars, selection, etc. 
     
     */
    template<typename T = Terminal>
    class TerminalUI : public virtual Widget {
    public:

        TerminalUI(T * terminal):
            history_{new TerminalHistory{terminal}},
            terminal_{terminal} {
            setLayout(new Layout::Column{});
            // attch the history and terminal widgets
            attach(history_);
            attach(terminal_);
        }
            

        /** \name Returns the actual terminal. 
         */
        //@{
        T const * terminal() const {
            return terminal_;
        }

        T * terminal() {
            return terminal_;
        }
        //@}

        /** \name Maximum number of history rows. 
         */
        //@{

        int maxHistoryRows() const {
            return history_->maxRows();
        }

        void setMaxHistoryRows(int value) {
            history_->setMaxRows(value);
        }
        //@}

    protected:

    private:

        TerminalHistory * history_;

        T * terminal_;

    }; // ui::TerminalUI<T>



#ifdef FOO

    template<typename T>
    class UIXTerminal : public T {



    protected:

        void alternateMode(bool enabled) override {
            cancelSelectionUpdate();
            clearSelection();
            schedule([this](){
                if (enabled)
                    setScrollOffset(Point{0, 0});
                else
                    setScrollOffset(Point{0, static_cast<int>(historyRows_.size())});
            });
        }



    };

        /** Returns the number of current history rows. 
         
            Note that to do so, the buffer must be locked as history rows are protected by its mutex so this function is not as cheap as getting a size of a vector. 
         */
        int historyRows() {
            std::lock_guard<PriorityLock> g{bufferLock_};
            return static_cast<int>(historyRows_.size());
        }

        int maxHistoryRows() const {
            return maxHistoryRows_;
        }

        void setMaxHistoryRows(int value) {
            if (value != maxHistoryRows_) {
                maxHistoryRows_ = std::max(value, 0);
                std::lock_guard<PriorityLock> g{bufferLock_};
                while (historyRows_.size() > static_cast<size_t>(maxHistoryRows_)) {
                    delete [] historyRows_.front().second;
                    historyRows_.pop_front();
                }
            }
        }

        void setScrollOffset(Point const & value) override {
            Widget::setScrollOffset(value);
            scrollToTerminal_ = value.y() == historyRows();
        }

        void addHistoryRow(Cell * row, int cols);

        void resizeHistory();

        /** Returns the top offset of the terminal buffer in the currently drawed. 
         */
        int terminalBufferTop() const {
            ASSERT(bufferLock_.locked());
            return alternateMode_ ? 0 : static_cast<int>(historyRows_.size());            
        }

        /** Converts the given widget coordinates to terminal buffer coordinates. 
         
         */
        Point toBufferCoords(Point const & widgetCoordinates) {
            ASSERT(bufferLock_.locked());
            return widgetCoordinates + scrollOffset() - Point{0, terminalBufferTop()};
        }

        /** Returns the cell at given coordinates. 
         
            The coordinates are adjusted for the scroll buffer and then either a terminal buffer, or history cell is returned. In case of history cells, it is possible that no cell exists at the coordinates if the particular line was terminated before, in which case nullptr is returned. 
         */
        Cell const * cellAt(Point coords);

        /** Returns previous cell coordinates in contents coords. (that left of current one)
         */
        Point prevCell(Point coords) const;

        /** Returns next cell coordinates in contents coords. (that right of current one)
         */
        Point nextCell(Point coords) const;

        /** On when the terminal is scrolled completely in view (i.e. past all history rows) and any history rows added will automatically scroll the terminal as well. 
         */
        bool scrollToTerminal_ = true;

        int maxHistoryRows_ = 0;
        std::deque<std::pair<int, Cell*>> historyRows_;

        Size contentsSize() const override {
            if (alternateMode_) {
                return Widget::contentsSize();
            } else {
                std::lock_guard<PriorityLock> g(bufferLock_.priorityLock(), std::adopt_lock);
                return Size{width(), height() + static_cast<int>(historyRows_.size())};
            }
        }

        std::string getSelectionContents() override;


        /** Selects the word under given coordinates, if any. 
         
            Words may be split across lines. 
         */
        void selectWord(Point pos);

        /** Selects the current line of test under given coordinates. 
         
            Uses the line ending marks to determine the extent of the line. 
         */
        void selectLine(Point pos);


        /** Double click selects word under caret. 
         
            Works only if the terminal does not capture mouse. 
         */
        void mouseDoubleClick(MouseButtonEvent::Payload & e) override {
            Widget::mouseDoubleClick(e);
            if (! mouseCaptured() && e.active()) {
                selectWord(toContentsCoords(e->coords));
            }
        }

        /** Triple click selects line under caret. 
         
            Works only if the terminal does not capture mouse. 
         */
        void mouseTripleClick(MouseButtonEvent::Payload & e) override {
            Widget::mouseTripleClick(e);
            if (! mouseCaptured() && e.active()) {
                selectLine(toContentsCoords(e->coords));
            }
        }

    AnsiTerminal::Cell const * AnsiTerminal::cellAt(Point coords) {
        ASSERT(bufferLock_.locked());
        int bufferTop = terminalBufferTop();
        if (bufferTop <= coords.y()) {
            coords -= Point{0, bufferTop};
            return & const_cast<Buffer const &>(state_->buffer).at(coords);
        } else {
            auto const & row = historyRows_[coords.y()];
            if (coords.x() >= row.first)
                return nullptr;
            return row.second + coords.x();
        }
    }

    Point AnsiTerminal::prevCell(Point coords) const {
        ASSERT(bufferLock_.locked());
        coords -= Point{1,0};
        if (coords.x() < 0)
            coords += Point{state_->buffer.width(), -1};
        return coords;
    }

    Point AnsiTerminal::nextCell(Point coords) const {
        ASSERT(bufferLock_.locked());
        coords += Point{1,0};
        if (coords.x() >= state_->buffer.width())
            coords -= Point{state_->buffer.width(), -1};
        return coords;
    }

    /** If the terminal is scrolled into view, scrolls the terminal into view after the history line has been added as well. 
     */
    void AnsiTerminal::addHistoryRow(Cell * row, int cols) {
        if (cols <= width()) {
            historyRows_.push_back(std::make_pair(cols, row));
        // if the line is too long, simply chop it in pieces of maximal length
        } else {
            Cell * i = row;
            while (cols != 0) {
                int xSize = std::min(width(), cols);
                Cell * x = new Cell[xSize];
                // copy the cells one by one as they are PODs
                MemCopy(x, i, xSize);
                i += xSize;
                cols -= xSize;
                historyRows_.push_back(std::make_pair(xSize, x));
            }
            delete [] row;
        }
        while (historyRows_.size() > static_cast<size_t>(maxHistoryRows_)) {
            delete [] historyRows_.front().second;
            historyRows_.pop_front();
        }
        if (scrollToTerminal_)
            schedule([this](){
                setScrollOffset(Point{0, static_cast<int>(historyRows_.size())});
            });
    }

    void AnsiTerminal::selectWord(Point pos) {
        Point start = pos;
        Point end = pos;
        {
            std::lock_guard<PriorityLock> g(bufferLock_);
            Cell const * c = cellAt(pos);
            // if there is nothing at the coordinates, or we are not inside a word, do nothing
            if (c == nullptr || IsWordSeparator(c->codepoint()))
                return;
            // find beginning and end of the word
            while (true) {
                Point prev = prevCell(start);
                c = cellAt(prev);
                if (c == nullptr || IsWordSeparator(c->codepoint()))
                    break;
                start = prev;
            }
            while(true) {
                Point next = nextCell(end);
                c = cellAt(next);
                if (c == nullptr || IsWordSeparator(c->codepoint()))
                    break;
                end = next;
            }
        }
        // do the selection
        setSelection(Selection::Create(start, end));
    }

    void AnsiTerminal::selectLine(Point pos) {
        Point start = Point{0, pos.y()};
        Point end = start;
        {
            std::lock_guard<PriorityLock> g(bufferLock_);
            // see if the above line ends with a line end character
            while (start != Point{0,0}) {
                start = prevCell(start);
                Cell const * c = cellAt(start);
                if (c != nullptr && Buffer::IsLineEnd(*c)) {
                    start = Point{0, start.y() + 1};
                    break;
                }
            }
            // now find end of the line at cursor
            Point bottomRight = Point{state_->buffer.width() - 1, state_->buffer.height() - 1 + terminalBufferTop()};
            while (end != bottomRight) {
                Cell const * c = cellAt(end);
                if (c != nullptr && Buffer::IsLineEnd(*c))
                    break;
                end = nextCell(end);
            }
        }
        // do the selection
        setSelection(Selection::Create(start, end));
    }

    std::string AnsiTerminal::getSelectionContents() {
        std::stringstream result;
        Selection sel = selection();
        int row = sel.start().y();
        int endRow = sel.end().y();
        int col = sel.start().x();
        std::lock_guard<PriorityLock> g(bufferLock_);
        int terminalTop =  alternateMode_ ? 0 : static_cast<int>(historyRows_.size());
        while (row < endRow) {
            int endCol = (row < endRow - 1) ? width() : sel.end().x();
            Cell * rowCells;
            // if the current row comes from the history, get the appropriate cells
            if (row < terminalTop) {
                rowCells = historyRows_[row].second;
                // if the stored row is shorter than the start of the selection, adjust the endCol so that no processing will be involved
                if (endCol > historyRows_[row].first)
                    endCol = historyRows_[row].first;
            } else {
                rowCells = state_->buffer.row(row - terminalTop);
            }
            // analyze the line and add it to the selection now
            std::stringstream line;
            for (; col < endCol; ) {
                line << Char{rowCells[col].codepoint()};
                if (Buffer::IsLineEnd(rowCells[col]))
                    line << std::endl;
                col += rowCells[col].font().width();
            }
            // remove whitespace at the end of the line if the line ends with enter
            std::string l{line.str()};
            if (! l.empty()) {
                size_t lineEnd = l.size();
                while (lineEnd > 0) {
                    --lineEnd;
                    if (l[lineEnd] == '\n')
                        break;
                    if (l[lineEnd] != ' ' && l[lineEnd] != '\t')
                        break;
                }
                if (l[lineEnd] == '\n')
                    result << l.substr(0, lineEnd + 1);
                else
                    result << l;
            }
            // do next row, all next rows start from 0
            ++row;
            col = 0;
        }
        return Trim(result.str());
    }

    void AnsiTerminal::mouseMove(MouseMoveEvent::Payload & e) {
        onMouseMove(e, this);
        if (e.active()) {
            Point bufferCoords;
            {
                std::lock_guard<PriorityLock> g(bufferLock_.priorityLock(), std::adopt_lock);
                bufferCoords = toBufferCoords(e->coords);
                Hyperlink * a = hyperlinkAt(e->coords);
                // if there is active hyperlink that is different from current special object, deactive it
                if (activeHyperlink_ != nullptr && activeHyperlink_ != a) {
                    activeHyperlink_->setActive(false);
                    activeHyperlink_ = nullptr;
                    repaint();
                    setMouseCursor(MouseCursor::Default);
                }
                if (activeHyperlink_ == nullptr && a != nullptr) {
                    activeHyperlink_ = a;
                    activeHyperlink_->setActive(true);
                    repaint();
                    setMouseCursor(MouseCursor::Hand);
                }
            }
            if (// the mouse movement should actually be reported
                (mouseMode_ == MouseMode::All || (mouseMode_ == MouseMode::ButtonEvent && mouseButtonsDown_ > 0)) && 
                // only send the mouse information if the mouse is in the range of the window
                bufferCoords.y() >= 0 && 
                Rect{size()}.contains(e->coords)) {
                    // mouse move adds 32 to the last known button press
                    sendMouseEvent(mouseLastButton_ + 32, bufferCoords, 'M');
                    LOG(SEQ) << "Mouse moved to " << e->coords << "(buffer coords " << bufferCoords << ")";
            } else {
                SelectionOwner::mouseMove(e);
            }
        }
    }

    void AnsiTerminal::mouseDown(MouseButtonEvent::Payload & e) {
        ++mouseButtonsDown_;
        onMouseDown(e, this);
        if (e.active()) {
            if (mouseMode_ != MouseMode::Off) {
                Point bufferCoords;
                {
                    std::lock_guard<PriorityLock> g(bufferLock_.priorityLock(), std::adopt_lock);
                    bufferCoords = toBufferCoords(e->coords);
                }
                if (bufferCoords.y() >= 0) {
                    mouseLastButton_ = encodeMouseButton(e->button, e->modifiers);
                    sendMouseEvent(mouseLastButton_, bufferCoords, 'M');
                    LOG(SEQ) << "Button " << e->button << " down at " << e->coords << "(buffer coords " << bufferCoords << ")";
                }
            } else {
                SelectionOwner::mouseDown(e);
            }
        }
    }

    void AnsiTerminal::mouseUp(MouseButtonEvent::Payload & e) {
        onMouseUp(e, this);
        if (e.active()) {
            // a bit of defensive programming
            if (mouseButtonsDown_ > 0) {
                --mouseButtonsDown_;
                if (mouseMode_ != MouseMode::Off) {
                    Point bufferCoords;
                    {
                        std::lock_guard<PriorityLock> g(bufferLock_.priorityLock(), std::adopt_lock);
                        bufferCoords = toBufferCoords(e->coords);
                    }
                    if (bufferCoords.y() >= 0) {
                        mouseLastButton_ = encodeMouseButton(e->button, e->modifiers);
                        sendMouseEvent(mouseLastButton_, bufferCoords, 'm');
                        LOG(SEQ) << "Button " << e->button << " up at " << e->coords << "(buffer coords " << bufferCoords << ")";
                    }
                } else {
                    SelectionOwner::mouseUp(e);
                }
            }
        }
    }

    void AnsiTerminal::mouseWheel(MouseWheelEvent::Payload & e) {
        onMouseWheel(e, this);
        if (e.active()) {
            if (! alternateMode_ && historyRows_.size() > 0) {
                if (e->by > 0)
                    scrollBy(Point{0, -1});
                else 
                    scrollBy(Point{0, 1});
            } else {
                if (mouseMode_ != MouseMode::Off) {
                    Point bufferCoords;
                    {
                        std::lock_guard<PriorityLock> g(bufferLock_.priorityLock(), std::adopt_lock);
                        bufferCoords = toBufferCoords(e->coords);
                    }
                    if (bufferCoords.y() >= 0) {
                        // mouse wheel adds 64 to the value
                        mouseLastButton_ = encodeMouseButton((e->by > 0) ? MouseButton::Left : MouseButton::Right, e->modifiers) + 64;
                        sendMouseEvent(mouseLastButton_, bufferCoords, 'M');
                        LOG(SEQ) << "Wheel offset " << e->by << " at " << e->coords << "(buffer coords " << bufferCoords << ")";
                    }
                }
            }
        }
    }

    void AnsiTerminal::keyDown(KeyEvent::Payload & e) {
        onKeyDown(e, this);
        if (e.active()) {
            // only scroll to prompt if the key down is not a simple modifier key, but don't do this in alternate mode when scrolling is disabled
            if (! alternateMode_ 
                && *e != Key::ShiftKey + Key::Shift 
                && *e != Key::AltKey + Key::Alt
                && *e != Key::CtrlKey + Key::Ctrl
                && *e != Key::WinKey + Key::Win)
                setScrollOffset(Point{0, historyRows()});
            auto i = KeyMap_.find(*e);
            // only emit keyDown for non-printable keys as printable keys will go through the keyCHar event
            if (i != KeyMap_.end() && PrintableKeys_.find(*e) == PrintableKeys_.end()) {
                std::string const * seq = &(i->second);
                if ((cursorMode_ == CursorMode::Application &&
                    e->modifiers() == Key::Invalid) && (
                    e->key() == Key::Up ||
                    e->key() == Key::Down || 
                    e->key() == Key::Left || 
                    e->key() == Key::Right || 
                    e->key() == Key::Home || 
                    e->key() == Key::End)) {
                        std::string sa(*seq);
                        sa[1] = 'O';
                        send(sa.c_str(), sa.size());
                } else {
                        send(seq->c_str(), seq->size());
                }
            }
        }
        // don't propagate to parent as the terminal handles keyboard input itself
    }


        /** Returns hyperlink attached to cell at given coordinates. 
         
            If there are no cells, at given coordinates, or no hyperlink present, returns nullptr.

            The coordinates are given in widget's contents coordinates.  
         */
        Hyperlink * hyperlinkAt(Point widgetCoords) {
            ASSERT(bufferLock_.locked());
            Cell const * cell = cellAt(toContentsCoords(widgetCoords));
            return cell == nullptr ? nullptr : dynamic_cast<Hyperlink*>(cell->specialObject());
        }



#endif

} // namespace ui
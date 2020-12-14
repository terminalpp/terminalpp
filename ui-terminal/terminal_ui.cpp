#include "helpers/memory.h"

#include "ansi_terminal.h"

#include "terminal_ui.h"

namespace ui {

    void TerminalHistory::addHistoryRow(Terminal::NewHistoryRowEvent::Payload & e) {
        ASSERT(e.sender() == terminal_);
        if (maxRows_ == 0 || e->buffer != Terminal::BufferKind::Normal)
            return;
        Terminal::Cell const * start = e->cells;
        Terminal::Cell const * end = e->cells + e->width - 1;
        // if we don't find end of line character, we must remember the whole line
        size_t lineWidth = e->width;
        Color defaultBg = terminal_->palette().defaultBackground();
        while (end > start) {
            // if we have found end of line character, good
            if (Terminal::Buffer::IsLineEnd(*end)) {
                lineWidth = end - start + 1;
                break;
            }
            // if we have found a visible character, we must remember the whole line, break the search - any end of line characters left of it will make no difference
            if (end->codepoint() != ' ' || end->font().underline() || end->font().strikethrough() || end->bg() != defaultBg)
                break;
            // otherwise move to previous column
            --end;
        }
        ASSERT(lineWidth <= e->width);
        Terminal::Cell * row = new Terminal::Cell[lineWidth];
        // we cannot use memcopy here because the cells can be special
        MemCopy(row, start, lineWidth);
        addHistoryRow(static_cast<int>(lineWidth), row);
        // TODO repaint & relayout
        // also determine if we should scroll the terminal in view or not
    }

    void TerminalHistory::resize(Size const & size) {
        if (rect().width() != size.width()) {
            std::deque<Row> oldRows{std::move(rows_)};
            Terminal::Cell * row = nullptr;
            int rowWidth = 0;
            for (Row & oldRow : oldRows) {
                if (row == nullptr) {
                    row = oldRow.cells;
                    oldRow.cells = nullptr;
                    rowWidth = oldRow.width;
                } else {
                    Terminal::Cell * newRow = new Terminal::Cell[rowWidth + oldRow.width];
                    MemCopy(newRow, row, rowWidth);
                    MemCopy(newRow + rowWidth, oldRow.cells, oldRow.width);
                    rowWidth += oldRow.width;
                    delete [] row;
                    row = newRow;
                }
                ASSERT(row != nullptr);
                if (Terminal::Buffer::IsLineEnd(row[rowWidth - 1])) {
                    addHistoryRow(rowWidth, row);
                    row = nullptr;
                    rowWidth = 0;
                }
            }
            if (row != nullptr)
                addHistoryRow(rowWidth, row);
        }
        Widget::resize(size);
    }


    void TerminalHistory::paint(Canvas & canvas) {
#ifdef SHOW_LINE_ENDINGS
        Border endOfLine{Border::All(Color::Red, Border::Kind::Thin)};
#endif
        Rect visibleRect{canvas.visibleRect()};
        // TODO lock
        // only display the rows that are in the visible rectangle
        for (int ri = visibleRect.top(), re = visibleRect.bottom(); ri < re; ++ri) {
            Row & row = rows_[ri];
            for (int ci = 0, ce = row.width; ci < ce; ++ci) {
                canvas.at(Point{ci, ri}).stripSpecialObjectAndAssign(row.cells[ci]);
#ifdef SHOW_LINE_ENDINGS
                if (AnsiTerminal::Buffer::IsLineEnd(row.cells[ci]))
                    canvas.setBorder(Point{ci, ri}, endOfLine);
#endif
            }
            // TODO should the rest of the line be drawn here?
        }
    }

}
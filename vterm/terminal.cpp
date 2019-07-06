
#include <cstring>

#include "helpers/log.h"

#include "terminal.h"
#include "pty.h"

namespace vterm {


    void Terminal::Screen::insertLines(unsigned lines, unsigned top, unsigned bottom, Cell fill) {
        ASSERT(bottom <= rows_);
        for (size_t i = 0; i < lines; ++i) {
            Cell * row = cells_[bottom];
            memmove(cells_ + 1, cells_, sizeof(Cell*) * (bottom - top - 1));
            cells_[top] = row;
            fillRow(row, fill);
        }
        for (size_t i = top; i < bottom; ++i)
            markRowDirty(i);
    }

    void Terminal::Screen::deleteLines(unsigned lines, unsigned top, unsigned bottom, Cell fill) {
        ASSERT(bottom <= rows_);
        for (size_t i = 0; i < lines; ++i) {
            Cell * row = cells_[top];
            memmove(cells_, cells_ + 1, sizeof(Cell*) * (bottom - top - 1));
            cells_[bottom - 1] = row;
            fillRow(row, fill);
        }
        for (size_t i = top; i < bottom; ++i)
            markRowDirty(i);
    }

    void Terminal::Screen::fillRow(Cell * row, Cell const & fill) {
        row[0] = fill;
        size_t i = 1;
        size_t next = 2;
        while (next < cols_) {
            memcpy(row + i, row, sizeof(Cell) * i);
            i = next;
            next *= 2;
        }
        memcpy(row + i, row, sizeof(Cell) * (cols_ - i));
    }

	std::string Terminal::getText(Selection const & selection) const	{
		std::stringstream result;
		unsigned col = selection.start.col;
		unsigned row = selection.start.row;
		// TODO this is ugly
		ScreenLock sl = const_cast<Terminal*>(this)->lockScreen();
		while (selection.contains(col, row) && col < sl->cols() && row < sl->rows()) {
			result << sl->at(col, row).c();
			if (++col == sl->cols()) {
				++row;
				col = 0;
				result << '\n';
			}
		}
		return result.str();
	}

} // namespace vterm
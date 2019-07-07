
#include <cstring>

#include "helpers/log.h"

#include "terminal.h"
#include "pty.h"

namespace vterm {

	void Terminal::Screen::resizeCells(unsigned newCols, unsigned newRows) {
		// create the new cells 
		Cell** newCells = new Cell * [newRows];
		for (size_t i = 0; i < newRows; ++i)
			newCells[i] = new Cell[newCols];
		bool* newDirtyRows = new bool[newRows];
		// now determine the row at which we should stop - this is done by going back from cursor's position until we hit end of line, that would be the last line we will use
		unsigned stopRow = cursor_.row - 1;
		while (stopRow < rows_) {
			Cell* row = cells_[stopRow];
			unsigned i = 0;
			for (; i < cols_; ++i)
				if (row[i].isLineEnd())
					break;
			// we have found the line end
			if (i < cols_) {
				++stopRow; // stop after current row
				break;
			}
			// otherwise try the above line
			--stopRow;
		}
		// if not found, or out of bounds, we'll stop immediately
		if (stopRow > rows_)
			stopRow = 0;
		// now transfer the contents
		unsigned oldCursorRow = cursor_.row;
		cursor_.col = 0;
		cursor_.row = 0;
		for (unsigned y = 0; y < stopRow; ++y) {
			for (unsigned x = 0; x < cols_; ++x) {
				Cell& cell = cells_[y][x];
				newCells[cursor_.row][cursor_.col] = cell;
				// if the copied cell is end of line, or if we are at the end of new line, increase the cursor row
				if (cell.isLineEnd() || ++cursor_.col == newCols) {
					++cursor_.row;
					cursor_.col = 0;
				}
				// scroll the new lines if necessary
				if (cursor_.row == newRows) {
					Cell* x = newCells[0];
					memmove(newCells, newCells + 1, sizeof(Cell*) * (newRows - 1));
					newCells[newRows - 1] = x;
					fillRow(x, Cell(), newCols);
					--cursor_.row;
				}
				// if it was new line, skip whatever was afterwards
				if (cell.isLineEnd())
					break;
			}
		}
		// the contents was transferred, delete the old cells, replace them with the new ones
		for (size_t i = 0; i < rows_; ++i)
			delete[] cells_[i];
		delete[] cells_;
		delete[] dirtyRows_;
		cells_ = newCells;
		dirtyRows_ = newDirtyRows;
		// because the first thing the app will do after resize is to adjust the cursor position if the current line span more than 1 terminal line, we must account for this and update cursor position
		cursor_.row += (oldCursorRow - stopRow);
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
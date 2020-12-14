#include "terminal.h"

namespace ui {

    // Palette

    Terminal::Palette Terminal::Palette::Colors16() {
		return Palette{
			Color::Black, // 0
			Color::DarkRed, // 1
			Color::DarkGreen, // 2
			Color::DarkYellow, // 3
			Color::DarkBlue, // 4
			Color::DarkMagenta, // 5
			Color::DarkCyan, // 6
			Color::Gray, // 7
			Color::DarkGray, // 8
			Color::Red, // 9
			Color::Green, // 10
			Color::Yellow, // 11
			Color::Blue, // 12
			Color::Magenta, // 13
			Color::Cyan, // 14
			Color::White // 15
		};
    }

    Terminal::Palette Terminal::Palette::XTerm256() {
        Palette result(256);
        // first the basic 16 colors
		result[0] =	Color::Black;
		result[1] =	Color::DarkRed;
		result[2] =	Color::DarkGreen;
		result[3] =	Color::DarkYellow;
		result[4] =	Color::DarkBlue;
		result[5] =	Color::DarkMagenta;
		result[6] =	Color::DarkCyan;
		result[7] =	Color::Gray;
		result[8] =	Color::DarkGray;
		result[9] =	Color::Red;
		result[10] = Color::Green;
		result[11] = Color::Yellow;
		result[12] = Color::Blue;
		result[13] = Color::Magenta;
		result[14] = Color::Cyan;
		result[15] = Color::White;
		// now do the xterm color cube
		unsigned i = 16;
		for (unsigned r = 0; r < 256; r += 40) {
			for (unsigned g = 0; g < 256; g += 40) {
				for (unsigned b = 0; b < 256; b += 40) {
					result[i] = Color(
						static_cast<unsigned char>(r),
						static_cast<unsigned char>(g),
						static_cast<unsigned char>(b)
					);
					++i;
					if (b == 0)
						b = 55;
				}
				if (g == 0)
					g = 55;
			}
			if (r == 0)
				r = 55;
		}
		// and finally do the grayscale
		for (unsigned char x = 8; x <= 238; x += 10) {
			result[i] = Color(x, x, x);
			++i;
		}
        return result;
    }

    Terminal::Palette::Palette(std::initializer_list<Color> colors, Color defaultFg, Color defaultBg):
        defaultFg_(defaultFg),
        defaultBg_(defaultBg) {
		for (Color c : colors)
            colors_.push_back(c);
    }

    // Buffer

    void Terminal::Buffer::insertLine(int top, int bottom, Cell const & fill) {
        Cell * x = rows_[bottom - 1];
        memmove(rows_ + top + 1, rows_ + top, sizeof(Cell*) * (bottom - top - 1));
        rows_[top] = x;
        fillRow(top, fill, 0, width());
    }

    void Terminal::Buffer::deleteLine(int top, int bottom, Cell const & fill) {
        Cell * x = rows_[top];
        terminal_->onNewHistoryRow(NewHistoryRowEvent::Payload{bufferKind_, width(), x}, terminal_);
        memmove(rows_ + top, rows_ + top + 1, sizeof(Cell*) * (bottom - top - 1));
        rows_[bottom - 1] = x;
        fillRow(bottom - 1, fill, 0, width());
    }

    void Terminal::Buffer::resize() {
        Size newSize = MinSize(terminal_->size());
        // don't do anything if the terminal size is identical to own
        if (newSize == size())
            return;
        // determine the line at which the cursor is, which can span multiple terminal lines if it is wrapped. This is important because the contents of the cursor line and all lines below is not being copied to the resized buffer as it should be rewritten by the terminal app
        int stopRow = getCursorRowWrappedStart();
        // first keep the old rows and size so that we can copy the data from it
        Cell ** oldRows = rows_;
        int oldWidth = width();
        int oldHeight = height();
        // move the old rows out and call basic buffer resize to adjust width and height, fill the buffer with given cell so that we do not have to deal with uninitialized cells later. 
        rows_ = nullptr;
        Canvas::Buffer::resize(newSize);
        // reset the scroll region
        scrollStart_ = 0;
        scrollEnd_ = height();
        // fill the entire buffer with the default cell
        this->fill(terminal_->defaultCell());
        // now copy the contents from the old buffer to the new buffer, line by line, char by char
        // this is where we will be writing to
        cursorPosition_ = Point{0,0};
        for (int row = 0; row < stopRow; ++row) {
            Cell * old = oldRows[row];
            for (int col = 0; col < oldWidth; ++col) {
                adjustedCursorPosition();
                // append the character from the old buffer
                rows_[cursorPosition_.y()][cursorPosition_.x()] = old[col];
                // if the cell is marked as end of line and the rest of the line are just whitespace characters then set position to new line and ignore the whitespace
                if (IsLineEnd(old[col]) && hasOnlyWhitespace(old, col + 1, oldWidth)) {
                    cursorPosition_ = Point{0, cursorPosition_.y() + 1};
                    break;
                }
                // otherwise update the position to point to next column
                cursorPosition_ += Point{1,0};
            }
        }
        // adjust the cursor position after the last character
        adjustedCursorPosition();
        // and delete the old rows
        for (int i = 0; i < oldHeight; ++i)
            delete [] oldRows[i];
        delete [] oldRows;
    }

    Terminal::Cell & Terminal::Buffer::addCharacter(char32_t codepoint) {
        Cell & cell = at(adjustedCursorPosition());
        lastCharacter_ = cursorPosition_;
        cell = currentCell_;
        cell.setCodepoint(codepoint);
        // what's left is to deal with corner cases, such as larger fonts & double width characters
        int columnWidth = Char::ColumnWidth(codepoint);
        // if the character's column width is 2 and current font is not double width, update to double width font
        if (columnWidth == 2 && ! cell.font().doubleWidth()) {
            //columnWidth = 1;
            cell.font().setDoubleWidth(true);
        }
        // advance the cursor position
        // TODO should this be 1;0 or column width 0?
        cursorPosition_ += Point{1, 0};
        // return the cell
        return cell;


        // TODO do double width & height characters properly for the per-line 

        /*
        // if the character's column width is 2 and current font is not double width, update to double width font
        // if the font's size is greater than 1, copy the character as required (if we are at the top row of double height characters, increase the size artificially)
        int charWidth = state_->doubleHeightTopLine ? cell.font().width() * 2 : cell.font().width();

        while (columnWidth > 0 && cursorPosition().x() < buffer_.width()) {
            for (int i = 1; (i < charWidth) && cursorPosition().x() < buffer_.width(); ++i) {
                Cell& cell2 = buffer_.at(buffer_.cursor().pos);
                // copy current cell properties
                cell2 = cell;
                // make sure the cell's font is normal size and width and display a space
                cell2.setCodepoint(' ').setFont(cell.font().setSize(1).setDoubleWidth(false));
                ++cursorPosition().x();
            } 
            if (--columnWidth > 0 && cursorPosition().x() < buffer_.width()) {
                Cell& cell2 = buffer_.at(buffer_.cursor().pos);
                // copy current cell properties
                cell2 = cell;
                cell2.setCodepoint(' ');
                ++cursorPosition().x();
            } 
        }
        */
    }

    void Terminal::Buffer::newLine() {
        // mark the last character as line end, if any
        if (lastCharacter_.x() >= 0)
            markAsLineEnd(lastCharacter_);
        // disable double width and height chars
        currentCell_.font().setSize(1).setDoubleWidth(false);
        // if the cursor is inside the buffer increase its row, otherwise the extra columns after the buffer edge will give us the new line effect when cursor position is adjusted
        if (cursorPosition_.x() < width())
            cursorPosition_ += Point{0, 1};
        adjustedCursorPosition();
        // update the cursor position as LF takes immediate effect
    }

    /** The algorithm is simple. Start at the row one above current cursor position. Then if we find an end of line character on that row, we know the next row was the first line of the cursor. If there is no end of line character, then the line is wordwrapped to the line after it so we check the line above, or if we get all the way to the top of the buffer its the first line by definition.  
     */
    int Terminal::Buffer::getCursorRowWrappedStart() const {
        int row = cursorPosition_.y() - 1;
        ASSERT(row < height() && row >= -1);
        while (row >= 0) {
            Cell * cells = rows_[row];
            for (int col = width(); col >= 0; --col) {
                if (IsLineEnd(cells[col]))
                    return row + 1;
            }
            --row;
        }
        return row + 1;
    }

    Point Terminal::Buffer::adjustedCursorPosition() {
        ASSERT(width() > 0);
        // first make sure that the X coordinate is properly withing the buffer, incrementing the y coordinate accordingly. If this happens at scroll region boundary, delete & add lines to history appropriately
        while (cursorPosition_.x() >= width()) {
            cursorPosition_ += Point{-width(), 1};
            if (cursorPosition_.y() == scrollEnd_) {
                deleteLine(scrollStart_, scrollEnd_, currentCell_);
                cursorPosition_ -= Point{0, 1};
            }
        }
        // if we are at scroll end, delete the line
        if (cursorPosition_.y() == scrollEnd_) {
            deleteLine(scrollStart_, scrollEnd_, currentCell_);
            cursorPosition_ -= Point{0, 1};
        }
        // make sure that the cursor position is within the buffer and if not, update its row to the bottom one (this could be if the scroll region is not set to the entire buffer, or if the cursor position was really wrong to begin with)
        if (cursorPosition_.y() >= height())
            cursorPosition_.setY(height() - 1);
        return cursorPosition_;
    }

    bool Terminal::Buffer::hasOnlyWhitespace(Cell * row, int from, int width) {
        for (; from < width; ++from)
            if (!Char::IsWhitespace(row[from].codepoint()))
                return false;
        return true;
    }



} // namespace ui
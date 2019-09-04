#include "helpers/string.h"

#include "terminal.h"

namespace vterm {

    // Terminal::Buffer

    Terminal::Buffer::Buffer(int cols, int rows):
        cols_{cols},
        rows_{rows},
        cells_{new Cell*[rows]} {
        for (int i = 0; i < rows; ++i)
            cells_[i] = new Cell[cols];
    }

    Terminal::Buffer::Buffer(Buffer const & from):
        cols_{from.cols_},
        rows_{from.rows_},
        cells_{new Cell*[from.rows_]},
        cursor_{from.cursor_} {
        for (int i = 0; i < rows_; ++i) {
            cells_[i] = new Cell[cols_];
            memcpy(cells_[i], from.cells_[i], sizeof(Cell) * cols_);
        }
    }

    Terminal::Buffer & Terminal::Buffer::operator = (Buffer const & other) {
        // self - check
        if (this == &other)
            return *this;
        if (cols_ != other.cols_ || rows_ != other.rows_) {
            for (int i = 0; i < rows_; ++i)
                delete [] cells_[i];
            delete[] cells_;
            cols_ = other.cols_;
            rows_ = other.rows_;
            cells_ = new Cell*[rows_];
            for (int i = 0; i < rows_; ++i) 
                cells_[i] = new Cell[cols_];
        }
        for (int i = 0; i < rows_; ++i) 
            memcpy(cells_[i], other.cells_[i], sizeof(Cell) * cols_);
        cursor_ = other.cursor_;
        return *this;
    }

    Terminal::Buffer::~Buffer() {
        for (int i = 0; i < rows_; ++i)
            delete [] cells_[i];
        delete [] cells_;
    }

    void Terminal::Buffer::resize(int cols, int rows) {
        if (cols_ != cols || rows_ != rows) {
            resizeCells(cols, rows);
            cols_ = cols;
            rows_ = rows;
        }
    }

    void Terminal::Buffer::insertLines(int lines, int top, int bottom, Cell const & fill) {
        ASSERT(bottom <= rows_);
        for (int i = 0; i < lines; ++i) {
            Cell* row = cells_[bottom - 1];
            memmove(cells_ + top + 1, cells_ + top, sizeof(Cell*) * (bottom - top - 1));
            cells_[top] = row;
            fillRow(row, fill, cols_);
        }
    }

    void Terminal::Buffer::deleteLines(int lines, int top, int bottom, Cell const & fill) {
        ASSERT(bottom <= rows_);
        ASSERT(lines <= bottom - top);
        for (int i = 0; i < lines; ++i) {
            Cell* row = cells_[top];
            memmove(cells_ + top, cells_ + top + 1, sizeof(Cell*) * (bottom - top - 1));
            cells_[bottom - 1] = row;
            fillRow(row, fill, cols_);
        }
    }

    void Terminal::Buffer::fillRow(Cell* row, Cell const& fill, unsigned cols) {
        row[0] = fill;
        size_t i = 1;
        size_t next = 2;
        while (next < cols) {
            memcpy(row + i, row, sizeof(Cell) * i);
            i = next;
            next *= 2;
        }
        memcpy(row + i, row, sizeof(Cell) * (cols - i));
    }

	void Terminal::Buffer::resizeCells(int newCols, int newRows) {
		// create the new cells 
		Cell** newCells = new Cell * [newRows];
		for (int i = 0; i < newRows; ++i)
			newCells[i] = new Cell[newCols];
		// now determine the row at which we should stop - this is done by going back from cursor's position until we hit end of line, that would be the last line we will use
		int stopRow = cursor_.pos.y - 1;
		while (stopRow >= 0) {
			Cell* row = cells_[stopRow];
			int i = 0;
			for (; i < cols_; ++i)
				if (row[i].attributes().endOfLine())
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
		if (stopRow < 0 )
			stopRow = 0;
		// now transfer the contents
		int oldCursorRow = cursor_.pos.y;
		cursor_.pos.x = 0;
		cursor_.pos.y = 0;
		for (int y = 0; y < stopRow; ++y) {
			for (int x = 0; x < cols_; ++x) {
				Cell& cell = cells_[y][x];
				newCells[cursor_.pos.y][cursor_.pos.x] = cell;
				// if the copied cell is end of line, or if we are at the end of new line, increase the cursor row
				if (cell.attributes().endOfLine() || ++cursor_.pos.x == newCols) {
					++cursor_.pos.y;
					cursor_.pos.x = 0;
				}
				// scroll the new lines if necessary
				if (cursor_.pos.y == newRows) {
					Cell* r = newCells[0];
					memmove(newCells, newCells + 1, sizeof(Cell*) * (newRows - 1));
					newCells[newRows - 1] = r;
					fillRow(r, Cell(), newCols);
					--cursor_.pos.y;
				}
				// if it was new line, skip whatever was afterwards
				if (cell.attributes().endOfLine())
					break;
			}
		}
		// the contents was transferred, delete the old cells, replace them with the new ones
		for (int i = 0; i < rows_; ++i)
			delete[] cells_[i];
		delete[] cells_;
		cells_ = newCells;
		// because the first thing the app will do after resize is to adjust the cursor position if the current line span more than 1 terminal line, we must account for this and update cursor position
		cursor_.pos.y += (oldCursorRow - stopRow);
	}




    // Terminal 

    Terminal::Terminal(int width, int height, PTY * pty, unsigned fps, size_t ptyBufferSize):
        buffer_{width, height},
        pty_{pty},
        fps_{fps},
        repaint_{false} {
        pty_->resize(width, height);
        ptyReader_ = std::thread([this, ptyBufferSize](){
            std::unique_ptr<char> holder(new char[ptyBufferSize]);
            char * ptyBuffer = holder.get();
            char * writeStart = holder.get();
            while (true) {
                size_t read = pty_->receive(writeStart, ptyBufferSize - (writeStart - ptyBuffer));
				// if 0 bytes were read, terminate the thread
                if (read == 0)
                    break;
				// otherwise add any pending data from previous cycle
                read += (writeStart - ptyBuffer);
                // process the input
                size_t processed = processInput(ptyBuffer, read);
                // if not everything was processed, copy the unprocessed part at the beginning and set writeStart_ accordingly
                if (processed != read) {
                    memcpy(ptyBuffer, ptyBuffer + processed, read - processed);
                    writeStart = ptyBuffer + read - processed;
                } else {
                    writeStart = ptyBuffer;
                }
            }
        });
        ptyListener_ = std::thread([this](){
            helpers::ExitCode ec = pty_->waitFor();
            this->ptyTerminated(ec);
        });
        repainter_ = std::thread([this](){
            while (fps_ > 0) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1000 / fps_));
                if (repaint_) {
                    repaint_ = false;
                    repaint();
                }
            }
        });
    }

    void Terminal::paint(ui::Canvas & canvas) {
        Buffer::Ptr buffer = this->buffer(/* priority */true);
        canvas.copyBuffer(0,0,* buffer);
        if (!selection_.empty()) {
            ui::Brush selBrush(ui::Color(192, 192, 255, 128));
            canvas.fill(selection_, selBrush); 
        }
        if (focused())
            canvas.setCursor(buffer->cursor());
        else
            canvas.setCursor(ui::Cursor::Invisible());
    }

    void Terminal::mouseDown(int col, int row, ui::MouseButton button, ui::Key modifiers) {
        if (modifiers == 0) {
            if (button == ui::MouseButton::Left) {
                if (!selection_.empty())
                    clearSelection();
                updateSelectionRegionStart(ui::Point(col, row));
                requestRepaint();
            } else if (button == ui::MouseButton::Wheel) {
                requestSelectionPaste();                
            } else if (button == ui::MouseButton::Right && ! selection_.empty()) {
                setClipboard(selectionContents());
                clearSelection();
                requestRepaint();
            }
        }
        Widget::mouseDown(col, row, button, modifiers);
    }

    void Terminal::mouseUp(int col, int row, ui::MouseButton button, ui::Key modifiers) {
        if (modifiers == 0) {
            if (button == ui::MouseButton::Left) {
                updateSelectionRegionStop();
                setSelection(selectionContents());
            }
        }
        Widget::mouseUp(col, row, button, modifiers);
    }

    void Terminal::mouseMove(int col, int row, ui::Key modifiers) {
        if (modifiers == 0) {
            if (updatingSelectionRegion()) {
                updateSelectionRegion(ui::Point(col, row));
                requestRepaint();
            }
        }
        Widget::mouseMove(col, row, modifiers);
    }

    std::string Terminal::selectionContents() {
        if (selection_.empty())
            return std::string{};
        std::string result;
        std::stringstream line;
        {
            // get the buffer pointer
            Buffer::Ptr buf = buffer();
            if (selection_.start().y + 1 == selection_.end().y) {
                ui::Point p = selection_.start();
                for (; p.x < selection_.end().x; ++p.x)
                    line << helpers::Char::FromCodepoint(buf->at(p.x, p.y).codepoint());
                result += helpers::TrimRight(line.str());
            } else {
                ui::Point p = selection_.start();
                for (; p.x < buf->cols(); ++p.x)
                    line << helpers::Char::FromCodepoint(buf->at(p.x, p.y).codepoint());
                result += helpers::TrimRight(line.str());
                for (p.y = selection_.start().y + 1; p.y < selection_.end().y - 1; ++p.y) {
                    line.str("");
                    line.clear();
                    line << std::endl;
                    for (p.x = 0; p.x < buf->cols(); ++p.x)
                        line << helpers::Char::FromCodepoint(buf->at(p.x, p.y).codepoint());
                    result += helpers::TrimRight(line.str());
                }
                line.str("");
                line.clear();
                line << std::endl;
                for (p.y = selection_.end().y - 1, p.x = 0; p.x < selection_.end().x; ++p.x)
                        line << helpers::Char::FromCodepoint(buf->at(p.x, p.y).codepoint());
                result += helpers::TrimRight(line.str());
            }
        }
        return result;
    }

} // namespace vterm
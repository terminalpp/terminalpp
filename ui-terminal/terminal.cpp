#include "helpers/string.h"

#include "terminal.h"

namespace ui {

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

    void Terminal::Buffer::resize(int cols, int rows, Terminal * terminal) {
        if (cols_ != cols || rows_ != rows) {
            resizeCells(cols, rows, terminal);
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

    std::string Terminal::Buffer::getLine(int line) {
        ASSERT(line >= 0 && line < rows_);
        Cell * row = cells_[line];
        std::stringstream result;
        for (int i = 0; i < cols_; ++i)
            result << helpers::Char::FromCodepoint(row[i].codepoint());
        return result.str();
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

	void Terminal::Buffer::resizeCells(int newCols, int newRows, Terminal * terminal) {
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
                    // scroll the line out so that it will be added to the terminal's history
                    if (terminal != nullptr)
                        terminal->lineScrolledOut(r, newCols);
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
        repaint_{false},
        scrollable_{true},
        scrollBarActive_{false},
        historySizeLimit_{0} {
        pty_->resize(width, height);
        ptyReader_ = std::thread([this, ptyBufferSize](){
            try {
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
                    size_t processed = read;
                    try {
                        processed = processInput(ptyBuffer, read);
                        // trigger the input processed event
                        onInput(this, InputProcessedEvent{ptyBuffer, processed});
                    } catch (std::exception const & e) {
                        onInputError(this, InputErrorEvent{ptyBuffer, processed, e.what()});
                    }
                    // if not everything was processed, copy the unprocessed part at the beginning and set writeStart_ accordingly
                    if (processed != read) {
                        memcpy(ptyBuffer, ptyBuffer + processed, read - processed);
                        writeStart = ptyBuffer + read - processed;
                        // check against buffer overflow here
                        if (writeStart == ptyBuffer + ptyBufferSize) {
                            LOG() << "Buffer overflow. Discarding";
                            writeStart = ptyBuffer;
                        }
                    } else {
                        writeStart = ptyBuffer;
                    }
                }
            } catch (...) {
                LOG() << "Error in reader";
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
                    // trigger immediate repaint by calling widget's repaint implementation
                    Widget::repaint();
                }
            }
        });
    }

    Terminal::~Terminal() {
        fps_ = 0;
        delete pty_;
        ptyReader_.join();
        ptyListener_.join();
        repainter_.join();
        while (! history_.empty())
            popHistoryLine();
    }

    /** When terminal size is updated, we must resize the underlying buffer and the PTY.
     */
    void Terminal::updateSize(int width, int height) {
        {
            Buffer::Ptr b = buffer(true); // grab priority lock
            b->resize(width, height, this);
        }
        resizeHistory(width);
        pty_->resize(width, height);
        // resize the client canvas
        setClientArea(width, buffer_.rows() + static_cast<int>(history_.size())); 
        ScrollBox::updateSize(width, height);
        Widget::updateSize(width, height);
    }



    void Terminal::paint(Canvas & canvas) {
        // determine the client canvas - either the proper client canvas if scrolling is available, or the child canvas
        Canvas clientCanvas{scrollable_ ? getClientCanvas(canvas) : canvas};
        int terminalOffset = scrollable_ ? static_cast<int>(history_.size()) : 0;
        // draw the terminal if it is visible
        if (! scrollable_ || scrollOffset().y + height() > terminalOffset ) {
            Buffer::Ptr buffer = this->buffer(/* priority */true);
            clientCanvas.copyBuffer(0, terminalOffset, *buffer);
            // draw the cursor too
            if (focused()) 
                clientCanvas.setCursor(buffer->cursor() + Point{0, terminalOffset});
            else 
                clientCanvas.setCursor(Cursor::Invisible());
        }
        // if the terminal is scrollable, and there is any history, the scrollbar must be drawn and the history (if there is any visible)
        if (scrollable_ && history_.size() > 0) {
           ASSERT(clientCanvas.height() == terminalOffset + height());
           for (int i = scrollOffset().y; i < terminalOffset; ++i) {
               clientCanvas.fill(Rect{0, i, width(), i + 1}, defaultBackground());
               Cell * row = history_[i].second;
               for (int col = 0, ce = history_[i].first; col < ce; ++col)
                   clientCanvas.set(Point{col, i}, row[col]);
           }
           drawVerticalScrollbarOverlay(canvas, scrollBarActive_ ? Color::Red.withAlpha(128) : Color::White.withAlpha(64), scrollBarActive_);
        }
        // paint the selection, if any
        paintSelection(clientCanvas);
        // and finally, if the terminal is not enabled, dim its window accordingly
        if (!enabled())
            canvas.fill(Rect(width(), height()), Brush(Color::Black.withAlpha(128)));
    }

    void Terminal::mouseDown(int col, int row, MouseButton button, Key modifiers) {
        if (modifiers == 0) {
            if (button == MouseButton::Left) {
                startSelectionUpdate(col, row);
            } else if (button == MouseButton::Wheel) {
                requestSelectionContents(); 
            } else if (button == MouseButton::Right && ! selection().empty()) {
                setClipboard();
                clearSelection();
            }
        }
        Widget::mouseDown(col, row, button, modifiers);
    }

    void Terminal::mouseUp(int col, int row, MouseButton button, Key modifiers) {
        if (modifiers == 0) {
            if (button == MouseButton::Left)
                endSelectionUpdate(col, row);
        }
        Widget::mouseUp(col, row, button, modifiers);
    }

    void Terminal::mouseMove(int col, int row, Key modifiers) {
        if (modifiers == 0) {
            if (SelectionOwner::updating()) {
                selectionUpdate(col, row);
                calculateVerticalAutoScroll(col, row);
            }
            bool x = col == width() - 1;
            if (x != scrollBarActive_) {
                scrollBarActive_ = x;
                repaint();
            }
        }
        Widget::mouseMove(col, row, modifiers);
    }

    /** TODO Selection works for CJK and double width characters, is a bit messy for double height characters, but not worth the time now, should be fixed when the selection is restructured and more pasting options are added. 
     
        Selection will insert spaces in double width characters which should span multiple columns as this feature is not supported by the renderer and so it is emulated in the terminal by inserting the spaces.

        TODO perhaps change this to not space, but something else so that we can ignore it from the selection? Or ignore this corner case entirely.
     */
    std::string Terminal::getSelectionContents() {
        Selection s = selection();
        std::stringstream result;
        {
            Buffer::Ptr buf = buffer(); // lock the buffer (and therefore the history as well)
            int bufferStart = scrollable_ ? static_cast<int>(history_.size()) : 0;
            for (int y = s.start().y, ye = s.end().y; y < ye; ++y) {
                // determine the start (inclusive) and end (exclusive) column for the selection on current row
                int cStart = (y == s.start().y) ? s.start().x : 0;
                int cEnd = (y == s.end().y - 1) ? s.end().x : buf->cols();
                // update the column end based on the size of the line (if reading from history)
                cEnd = std::min(cEnd, (y >= bufferStart) ? buf->cols() : history_[y].first);
                // get the array of cells that corresponds to the current line (be it from buffer or history)
                Cell const * rowCells = (y >= bufferStart) ? & buf->at(0, y - bufferStart) : history_[y].second;
                
                // now create a UTF-8 string for the current line
                std::stringstream line;
                for (int i = cStart; i < cEnd; ) {
                    line << helpers::Char::FromCodepoint(rowCells[i].codepoint());
                    i += rowCells[i].font().width();
                }
                // trim the line if it ended with empty characters
                result << helpers::TrimRight(line.str()) << std::endl;
            }
        }
        return result.str();
    }

    void Terminal::lineScrolledOut(Cell * line, int cols) {
        if (! scrollable_) 
            return;
        if (historySizeLimit_ > 0) {
            addHistoryLine(line, cols);
            if (history_.size() > historySizeLimit_) {
                popHistoryLine();
            } else {
                updateClientRect();
                // TODO only scroll if we were at the top
                setScrollOffset(scrollOffset() + Point{0, 1});
            }
        }

        if (onLineScrolledOut.attached()) {
            buffer_.unlock();
            onLineScrolledOut(this);
            buffer_.lock();
        }
    }

    void Terminal::popHistoryLine() {
        ASSERT(! history_.empty());
        delete [] history_.front().second;
        history_.pop_front();
    }

    void Terminal::addHistoryLine(Cell const * line, int cols) {
        int x = cols - 1;
        Color bg = defaultBackground();
        while (x > 0) {
            Cell const & c = line[x];
            if (c.codepoint() != ' ')
                break;
            if (c.background() != bg)
                break;
            --x;
        }
        ++x;
        Cell * row = new Cell[x];
        memcpy(row, line, sizeof(Cell) * x);
        history_.push_back(std::make_pair(x, row));
    }

    void Terminal::resizeHistory(int newCols) {
        for (auto i = history_.begin(), e = history_.end(); i != e; ) {
            if (i->first > newCols) {
                int restSize = i->first - newCols;
                Cell * line = new Cell[newCols];
                Cell * rest = new Cell[restSize];
                memcpy(line, i->second, sizeof(Cell) * newCols);
                memcpy(rest, i->second + newCols, sizeof(Cell) * restSize);
                delete [] i->second;
                i->first = newCols;
                i->second = line;
                i = history_.insert(++i, std::make_pair(restSize, rest));
                e = history_.end(); // all iterators invalidated by insert above
            } else {
                ++i;
            }
        }
        while (history_.size() > historySizeLimit_)
            popHistoryLine();
    }

} // namespace vterm
#include <functional>
#include "ansi_terminal.h"

/** Inside debug builds, end of line is highlighted by red border.
 */
#ifndef NDEBUG
#define SHOW_LINE_ENDINGS
#endif

namespace ui {

    namespace {

        void InitializeKeyMap(std::unordered_map<Key, std::string> & keyMap) {
#define KEY(K, ...) { std::string x = STR(__VA_ARGS__); keyMap.insert(std::make_pair(K, x)); }
#include "ansi_keys.inc.h"
        }

        void InitializePrintableKeys(std::unordered_set<Key> & printableKeys) {
            for (unsigned k = 'A'; k <= 'Z'; ++k) {
                printableKeys.insert(Key::FromCode(k));
                printableKeys.insert(Key::FromCode(k) + Key::Shift);
            }
            for (unsigned k = '0'; k <= '9'; ++k) {
                printableKeys.insert(Key::FromCode(k));
            }
        }
    }

    std::unordered_map<Key, std::string> AnsiTerminal::KeyMap_;
    std::unordered_set<Key> AnsiTerminal::PrintableKeys_;


    Log AnsiTerminal::SEQ("VT100");
    Log AnsiTerminal::SEQ_UNKNOWN("VT100_UNKNOWN");
    Log AnsiTerminal::SEQ_ERROR("VT100_ERROR");
    Log AnsiTerminal::SEQ_WONT_SUPPORT("VT100_WONT_SUPPORT");
    Log AnsiTerminal::SEQ_SENT("VT100_SENT");

    char32_t AnsiTerminal::LineDrawingChars_[15] = {0x2518, 0x2510, 0x250c, 0x2514, 0x253c, 0, 0, 0x2500, 0, 0, 0x251c, 0x2524, 0x2534, 0x252c, 0x2502};

    AnsiTerminal::AnsiTerminal(tpp::PTYMaster * pty, Palette * palette):
        PTYBuffer{pty},
        palette_{palette},
        state_{new State{palette->defaultBackground()}},
        stateBackup_{new State{palette->defaultBackground()}} {
        if (KeyMap_.empty()) {
            InitializeKeyMap(KeyMap_);
            InitializePrintableKeys(PrintableKeys_);
        }
        state_->reset(palette_->defaultForeground(), palette_->defaultBackground());
        stateBackup_->reset(palette_->defaultForeground(), palette_->defaultBackground());
        setFocusable(true);
        
        startPTYReader();

    }

    AnsiTerminal::~AnsiTerminal() {
        terminatePty();
        delete palette_;
        delete state_;
        delete stateBackup_;
    }

    // Widget

    void AnsiTerminal::paint(Canvas & canvas) {
        Canvas ccanvas{contentsCanvas(canvas)};
#ifdef SHOW_LINE_ENDINGS
        Border endOfLine{Border::All(Color::Red, Border::Kind::Thin)};
#endif
        Rect visibleRect{ccanvas.visibleRect()};
        std::lock_guard<PriorityLock> g(bufferLock_.priorityLock(), std::adopt_lock);
        int top = alternateMode_ ? 0 : static_cast<int>(historyRows_.size());
        ccanvas.setBg(palette_->defaultBackground());
        // see if there are any history lines that need to be drawn
        for (int row = std::max(0, visibleRect.top()), re = std::min(top, visibleRect.bottom()); ; ++row) {
            if (row >= re)
                break;
            for (int col = 0, ce = historyRows_[row].first; col < ce; ++col) {
                ccanvas.at(Point{col, row}) = historyRows_[row].second[col];
#ifdef SHOW_LINE_ENDINGS
                if (Buffer::IsLineEnd(historyRows_[row].second[col]))
                    ccanvas.setBorder(Point{col, row}, endOfLine);
#endif
            }
            ccanvas.fill(Rect{Point{historyRows_[row].first, row}, Point{width(), row + 1}});
        }
        // now draw the actual buffer
        ccanvas.drawBuffer(state_->buffer, Point{0, top});
#ifdef  SHOW_LINE_ENDINGS
        // now add borders to the cells that are marked as end of line
        for (int row = std::max(top, visibleRect.top()), re = visibleRect.bottom(); ; ++row) {
            if (row >= re)
                break;
            for (int col = 0; col < width(); ++col) {
                if (Buffer::IsLineEnd(const_cast<Buffer const &>(state_->buffer).at(Point{col, row - top})))
                    ccanvas.setBorder(Point{col, row}, endOfLine);
            }
        }
#endif
        // draw the selection, if any 
        SelectionOwner::paint(ccanvas);
        // display scrollbars
        canvas.verticalScrollbar(top + height(), scrollOffset().y());
        // draw the cursor 
        if (focused()) {
            // set the cursor via the canvas
            ccanvas.setCursor(cursor(), cursorPosition() + Point{0, top});
        } else if (cursor().visible()) {
            // TODO the color of this should be configurable
            ccanvas.setBorder(cursorPosition() + Point{0, top}, Border::All(inactiveCursorColor_, Border::Kind::Thin));
        }
    }

    // User Input
    
    void AnsiTerminal::pasteContents(std::string const & contents) {
        if (bracketedPaste_) {
            send("\033[200~", 6);
            send(contents.c_str(), contents.size());
            send("\033[201~", 6);
        } else {
            send(contents.c_str(), contents.size());
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

    void AnsiTerminal::keyUp(KeyEvent::Payload & e) {
        onKeyUp(e, this);
        // don't propagate to parent as the terminal handles keyboard input itself
    }

    void AnsiTerminal::keyChar(KeyCharEvent::Payload & e) {
        onKeyChar(e, this);
        if (e.active()) {
            ASSERT(e->codepoint() >= 32);
            send(e->toCharPtr(), e->size());
        }
        // don't propagate to parent as the terminal handles keyboard input itself
    }

    void AnsiTerminal::mouseMove(MouseMoveEvent::Payload & e) {
        onMouseMove(e, this);
        if (e.active()) {
            if (// the mouse movement should actually be reported
                (mouseMode_ == MouseMode::All || (mouseMode_ == MouseMode::ButtonEvent && mouseButtonsDown_ > 0)) && 
                // only send the mouse information if the mouse is in the range of the window
                Rect{size()}.contains(e->coords)) {
                    // mouse move adds 32 to the last known button press
                    sendMouseEvent(mouseLastButton_ + 32, e->coords, 'M');
                    LOG(SEQ) << "Mouse moved to " << e->coords;
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
                mouseLastButton_ = encodeMouseButton(e->button, e->modifiers);
                sendMouseEvent(mouseLastButton_, e->coords, 'M');
                LOG(SEQ) << "Button " << e->button << " down at " << e->coords;
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
                    mouseLastButton_ = encodeMouseButton(e->button, e->modifiers);
                    sendMouseEvent(mouseLastButton_, e->coords, 'm');
                    LOG(SEQ) << "Button " << e->button << " up at " << e->coords;
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
                    // mouse wheel adds 64 to the value
                    mouseLastButton_ = encodeMouseButton((e->by > 0) ? MouseButton::Left : MouseButton::Right, e->modifiers) + 64;
                    sendMouseEvent(mouseLastButton_, e->coords, 'M');
                    LOG(SEQ) << "Wheel offset " << e->by << " at " << e->coords;
                }
            }
        }
    }

    unsigned AnsiTerminal::encodeMouseButton(MouseButton btn, Key modifiers) {
		unsigned result =
			((modifiers & Key::Shift) ? 4 : 0) +
			((modifiers & Key::Alt) ? 8 : 0) +
			((modifiers & Key::Ctrl) ? 16 : 0);
		switch (btn) {
			case MouseButton::Left:
				return result;
			case MouseButton::Right:
				return result + 1;
			case MouseButton::Wheel:
				return result + 2;
			default:
				UNREACHABLE;
		}
    }

    void AnsiTerminal::sendMouseEvent(unsigned button, Point coords, char end) {
		// first increment col & row since terminal starts from 1
        coords += Point{1,1};
		switch (mouseEncoding_) {
			case MouseEncoding::Default: {
				// if the event is release, button number is 3
				if (end == 'm')
					button |= 3;
				// increment all values so that we start at 32
				button += 32;
                coords += Point{32, 32};
				// if the col & row are too high, ignore the event
				if (coords.x() > 255 || coords.y() > 255)
					return;
				// otherwise output the sequence
				char buffer[6];
				buffer[0] = '\033';
				buffer[1] = '[';
				buffer[2] = 'M';
				buffer[3] = button & 0xff;
				buffer[4] = static_cast<char>(coords.x());
				buffer[5] = static_cast<char>(coords.y());
				send(buffer, 6);
				break;
			}
			case MouseEncoding::UTF8: {
				LOG(SEQ_WONT_SUPPORT) << "utf8 mouse encoding";
				break;
			}
			case MouseEncoding::SGR: {
				std::string buffer = STR("\033[<" << button << ';' << coords.x() << ';' << coords.y() << end);
				send(buffer.c_str(), buffer.size());
				break;
			}
		}
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
        return result.str();
    }

    // Terminal State 

    void AnsiTerminal::deleteCharacters(unsigned num) {
		int r = cursorPosition().y();
		for (unsigned c = cursorPosition().x(), e = state_->buffer.width() - num; c < e; ++c) 
			state_->buffer.at(c, r) = state_->buffer.at(c + num, r);
		for (unsigned c = state_->buffer.width() - num, e = state_->buffer.width(); c < e; ++c)
			state_->buffer.at(c, r) = state_->cell;
    }

    void AnsiTerminal::insertCharacters(unsigned num) {
		unsigned r = cursorPosition().y();
		// first copy the characters
		for (unsigned c = state_->buffer.width() - 1, e = cursorPosition().x() + num; c >= e; --c)
			state_->buffer.at(c, r) = state_->buffer.at(c - num, r);
		for (unsigned c = cursorPosition().x(), e = cursorPosition().x() + num; c < e; ++c)
			state_->buffer.at(c, r) = state_->cell;
    }

    void AnsiTerminal::updateCursorPosition() {
        while (cursorPosition().x() >= state_->buffer.width()) {
            ASSERT(state_->buffer.width() > 0);
            setCursorPosition(cursorPosition() - Point{state_->buffer.width(), -1});
            // if the cursor is on the last line, evict the lines above
            if (cursorPosition().y() == state_->scrollEnd) 
                deleteLines(1, state_->scrollStart, state_->scrollEnd, state_->cell);
        }
        if (cursorPosition().y() >= state_->buffer.height())
            setCursorPosition(Point{cursorPosition().x(), state_->buffer.height() - 1 });
        // the cursor position must be valid now
        ASSERT(cursorPosition().x() < state_->buffer.width());
        ASSERT(cursorPosition().y() < state_->buffer.height());
        // set last character position to the now definitely valid cursor coordinates
        state_->setLastCharacter(cursorPosition());
    }

    // Scrollback buffer

    void AnsiTerminal::insertLines(int lines, int top, int bottom, Cell const & fill) {
        while (lines-- > 0)
            state_->buffer.insertLine(top, bottom, fill);
    }

    /** If history is enabled, i.e. when history limit is greater than 0 and the terminal is not in alternate mode, the deleted line is added to the history. 
     */
    void AnsiTerminal::deleteLines(int lines, int top, int bottom, Cell const & fill) {
        // scroll the lines
        while (lines-- > 0) {
            if (! alternateMode_ && maxHistoryRows_ != 0) {
                auto removedRow = state_->buffer.copyRow(top, palette_->defaultBackground());
                addHistoryRow(removedRow.first, removedRow.second);
            }
            state_->buffer.deleteLine(top, bottom, fill);
        }
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
                memcpy(x, i, sizeof(Cell) * xSize); 
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

    void AnsiTerminal::resizeHistory() {
        std::deque<std::pair<int, Cell*>> oldRows{std::move(historyRows_)};
        Cell * row = nullptr;
        int rowSize = 0;
        for (auto & i : oldRows) {
            if (row == nullptr) {
                row = i.second;
                rowSize = i.first;
            } else {
                Cell * newRow = new Cell[rowSize + i.first];
                memcpy(newRow, row, sizeof(Cell) * rowSize);
                memcpy(newRow + rowSize, i.second, sizeof(Cell) * i.first);
                rowSize += i.first;
                delete [] i.second;
                delete [] row;
                row = newRow;
            }
            ASSERT(row != nullptr);
            if (Buffer::IsLineEnd(row[rowSize - 1])) {
                addHistoryRow(row, rowSize);
                row = nullptr;
                rowSize = 0;
            }
        }
        if (row != nullptr)
            addHistoryRow(row, rowSize);
    }

    void AnsiTerminal::resizeBuffers(Size size) {
        if (alternateMode_) {
            state_->resize(size, nullptr);
            stateBackup_->resize(size, std::bind(&AnsiTerminal::addHistoryRow, this, std::placeholders::_1, std::placeholders::_2));
        } else {
            state_->resize(size, std::bind(&AnsiTerminal::addHistoryRow, this, std::placeholders::_1, std::placeholders::_2));
            stateBackup_->resize(size, nullptr);
        }
    }



    // Input Processing

    size_t AnsiTerminal::received(char * buffer, char const * bufferEnd) {
        {
            std::lock_guard<PriorityLock> g(bufferLock_);
            // then process the input
            char const * x = buffer;
            while (x != bufferEnd) {
                switch (*x) {
                    /* Parse the escape sequence */
                    case Char::ESC: {
                        size_t processed = parseEscapeSequence(x, bufferEnd);
                        // if no characters were processed, the sequence was incomplete and we should end processing
                        if (processed == 0)
                            return x - buffer;
                        // move past the sequence
                        x += processed;
                        break;
                    }
                    /* BEL triggers the notification */
                    case Char::BEL:
                        parseNotification();
                        ++x;
                        break;
                    case Char::TAB:
                        parseTab();
                        ++x;
                        break;
                    case Char::LF:
                        parseLF();
                        ++x;
                        break;
                    case Char::CR:
                        parseCR();
                        ++x;
                        break;
                    case Char::BACKSPACE:
                        parseBackspace();
                        ++x;
                        break;
                    default: {
                        // while this is a code duplication from the Char class, since this code is a bottleneck for processing large ammounts of text, the code is copied for performance
                        char32_t cp = 0;
                        unsigned char const * ux = pointer_cast<unsigned char const *>(x);
                        if (*ux < 0x80) {
                            cp = *ux;
                            ++x;
                        } else if (*ux < 0xe0) {
                            if (x + 2 > bufferEnd)
                                return x - buffer;
                            cp = ((ux[0] & 0x1f) << 6) + (ux[1] & 0x3f);
                            x += 2;
                        } else if (*ux < 0xf0) {
                            if (x + 3 > bufferEnd)
                                return x - buffer;
                            cp = ((ux[0] & 0x0f) << 12) + ((ux[1] & 0x3f) << 6) + (ux[2] & 0x3f);
                            x += 3;
                        } else {
                            if (x + 4 > bufferEnd)
                                return x - buffer;
                            cp = ((ux[0] & 0x07) << 18) + ((ux[1] & 0x3f) << 12) + ((ux[2] & 0x3f) << 6) + (ux[3] & 0x3f);
                            x += 4;
                        }
                        parseCodepoint(cp);
                        break;
                    }
                }
            }
        }
        scheduleRepaint();
        return bufferEnd - buffer;
    }


    void AnsiTerminal::parseCodepoint(char32_t codepoint) {
        if (lineDrawingSet_ && codepoint >= 0x6a && codepoint < 0x79)
            codepoint = LineDrawingChars_[codepoint-0x6a];
        LOG(SEQ) << "codepoint " << codepoint << " " << static_cast<char>(codepoint & 0xff);
        updateCursorPosition();
        // set the cell state
        //Cell & cell = state_->buffer.set(cursorPosition(), state_->cell, codepoint);
        Cell & cell = state_->buffer.at(cursorPosition());
        cell = state_->cell;
        cell.setCodepoint(codepoint);
        // advance cursor's column
        setCursorPosition(cursorPosition() + Point{1, 0});

        // what's left is to deal with corner cases, such as larger fonts & double width characters
        int columnWidth = Char::ColumnWidth(codepoint);

        // if the character's column width is 2 and current font is not double width, update to double width font
        if (columnWidth == 2 && ! cell.font().doubleWidth()) {
            //columnWidth = 1;
            cell.font().setDoubleWidth(true);
        }

        // TODO do double width & height characters properly for the per-line 

        /*
        // if the character's column width is 2 and current font is not double width, update to double width font
        // if the font's size is greater than 1, copy the character as required (if we are at the top row of double height characters, increase the size artificially)
        int charWidth = state_->doubleHeightTopLine ? cell.font().width() * 2 : cell.font().width();

        while (columnWidth > 0 && cursorPosition().x() < state_->buffer.width()) {
            for (int i = 1; (i < charWidth) && cursorPosition().x() < state_->buffer.width(); ++i) {
                Cell& cell2 = buffer_.at(buffer_.cursor().pos);
                // copy current cell properties
                cell2 = cell;
                // make sure the cell's font is normal size and width and display a space
                cell2.setCodepoint(' ').setFont(cell.font().setSize(1).setDoubleWidth(false));
                ++cursorPosition().x();
            } 
            if (--columnWidth > 0 && cursorPosition().x() < state_->buffer.width()) {
                Cell& cell2 = buffer_.at(buffer_.cursor().pos);
                // copy current cell properties
                cell2 = cell;
                cell2.setCodepoint(' ');
                ++cursorPosition().x();
            } 
        }
        */
    }

    void AnsiTerminal::parseNotification() {
        schedule([this](){
            VoidEvent::Payload p;
            onNotification(p, this);
        });
    }

    void AnsiTerminal::parseTab() {
        updateCursorPosition();
        if (cursorPosition().x() % 8 == 0)
            setCursorPosition(cursorPosition() + Point{8, 0});
        else
            setCursorPosition(cursorPosition() + Point{8 - cursorPosition().x() % 8, 0});
        LOG(SEQ) << "Tab: cursor col is " << cursorPosition().x();
    }

    void AnsiTerminal::parseLF() {
        LOG(SEQ) << "LF";
        state_->markLineEnd();
        // disable double width and height chars
        state_->cell.font().setSize(1).setDoubleWidth(false);
        setCursorPosition(cursorPosition() + Point{0, 1});
        // determine if region should be scrolled
        if (cursorPosition().y() == state_->scrollEnd) {
            deleteLines(1, state_->scrollStart, state_->scrollEnd, state_->cell);
            setCursorPosition(cursorPosition() - Point{0, 1});
        }
        // update the cursor position as LF takes immediate effect
        updateCursorPosition();
    }

    void AnsiTerminal::parseCR() {
        LOG(SEQ) << "CR";
        // mark the last character as line end? 
        // TODO
        setCursorPosition(Point{0, cursorPosition().y()});
    }

    void AnsiTerminal::parseBackspace() {
        LOG(SEQ) << "BACKSPACE";
        if (cursorPosition().x() == 0) {
            if (cursorPosition().y() > 0)
                setCursorPosition(cursorPosition() - Point{0, 1});
            setCursorPosition(Point{state_->buffer.width() - 1, cursorPosition().y()});
        } else {
            setCursorPosition(cursorPosition() - Point{1, 0});
        }
    }

    size_t AnsiTerminal::parseEscapeSequence(char const * buffer, char const * bufferEnd) {
        ASSERT(*buffer == Char::ESC);
        // if we have nothing after the escape character, it is incomplete sequence
        char const * x = buffer;
        if (++x == bufferEnd)
            return 0;
        switch (*x++) {
            /* CSI Sequence. */
            case '[': {
                x -= 2;
                CSISequence seq{CSISequence::Parse(x, bufferEnd)};
                // if the sequence is not valid, it has been reported already and we should just exit
                if (!seq.valid())
                    break;
                // if the sequence is not complete, return false and do not advance the buffer
                if (!seq.complete())
                    return false;
                // otherwise parse the CSI sequence
                parseCSISequence(seq);
                break;
            }
            /* OSC (Operating System Command) */
            case ']': {
                x -= 2;
                OSCSequence seq{OSCSequence::Parse(x, bufferEnd)};
                // if the sequence is not valid, it has been reported already and we should just exit
                if (!seq.valid())
                    break;
                // if the sequence is not complete, return false and do not advance the buffer
                if (!seq.complete())
                    return false;
                parseOSCSequence(seq);
				break;
            }
			/* Save Cursor. */
			case '7':
				LOG(SEQ) << "DECSC: Cursor position saved";
                state_->saveCursor();
				break;
			/* Restore Cursor. */
			case '8':
                LOG(SEQ) << "DECRC: Cursor position restored";
                state_->restoreCursor();
				break;
			/* Reverse line feed - move up 1 row, same column.
			 */
			case 'M':
				LOG(SEQ) << "RI: move cursor 1 line up";
				if (cursorPosition().y() == state_->scrollStart) 
					insertLines(1, state_->scrollStart, state_->scrollEnd, state_->cell);
				else
                    setCursorPosition(cursorPosition() - Point{0, 1});
				break;
            /* Device Control String (DCS). 
             */
            case 'P':
                if (x == bufferEnd)
                    return false;
                if (*x == '+') {
                    // free the UI threadee
                    bufferLock_.unlock();
                    size_t p = parseTppSequence(buffer, bufferEnd);
                    bufferLock_.lock();
                    return p;
                } else {
                    LOG(SEQ_UNKNOWN) << "Unknown DCS sequence";
                }
                break;
    		/* Character set specification - most cases are ignored, with the exception of the box drawing and reset to english (0 and B) respectively. 
             */
			case '(':
                if (x != bufferEnd) {
                    if (*x == '0') {
                        ++x;
                        lineDrawingSet_ = true;
                        LOG(SEQ) << "Line drawing set selected";
                        break;
                    } else if (*x == 'B') {
                        ++x;
                        lineDrawingSet_ = false;
                        LOG(SEQ) << "Normal character set selected";
                        break;
                    }
                }
                // fallthrough
			case ')':
			case '*':
			case '+':
				// missing character set specification
				if (x == bufferEnd)
					return false;
				if (*x == 'B') { // US
					++x;
					break;
				}
				LOG(SEQ_WONT_SUPPORT) << "Unknown (possibly mismatched) character set final char " << *x;
				++x;
				break;
			/* ESC = -- Application keypad */
			case '=':
				LOG(SEQ) << "Application keypad mode enabled";
                keypadMode_ = KeypadMode::Application;
				break;
			/* ESC > -- Normal keypad */
			case '>':
				LOG(SEQ) << "Normal keypad mode enabled";
                keypadMode_ = KeypadMode::Normal;
				break;
            /* ESC # number -- font size changes */
            /*
            case '#':
                if (x == bufferEnd)
                    return false;
                parseFontSizeSpecifier(*x);
                ++x;
                break;
                */
            default:
				LOG(SEQ_UNKNOWN) << "Unknown escape sequence \x1b" << *(x-1);
				break;
        }
        return x - buffer;
    }

    size_t AnsiTerminal::parseTppSequence(char const * buffer, char const * bufferEnd) {
        // we know that we have at least \033P+   
        char const * i = buffer + 3;
        char const * tppEnd = tpp::Sequence::FindSequenceEnd(i, bufferEnd);
        // if not found, we need more data
        if (tppEnd == bufferEnd) 
            return 0;
        tpp::Sequence::Kind kind = tpp::Sequence::ParseKind(i, bufferEnd);
        // now we have kind and beginning and end of the payload so we can process the sequence
        LOG(SEQ) << "t++ sequence " << kind << ", payload size " << (tppEnd - i);
        // processes the tpp sequence, the default implementation simply raises the tpp sequence event
        tppSequence(TppSequenceEvent::Payload{kind, i, tppEnd});
        // and return
        return (tppEnd - buffer) + 1; // also process the BEL character at the end of the t++ sequence
    }

    void AnsiTerminal::parseCSISequence(CSISequence & seq) {
        switch (seq.firstByte()) {
            // the "normal" CSI sequences
            case 0: 
                switch (seq.finalByte()) {
                    // CSI <n> @ -- insert blank characters (ICH)
                    case '@':
                        seq.setDefault(0, 1);
                        LOG(SEQ) << "ICH: deleteCharacter " << seq[0];
                        insertCharacters(seq[0]);
                        return;
                    // CSI <n> A -- moves cursor n rows up (CUU)
                    case 'A': {
                        seq.setDefault(0, 1);
                        if (seq.numArgs() != 1)
                            break;
                        int r = cursorPosition().y() >= seq[0] ? cursorPosition().y() - seq[0] : 0;
                        LOG(SEQ) << "CUU: setCursor " << cursorPosition().x() << ", " << r;
                        setCursorPosition(Point{cursorPosition().x(), r});
                        return;
                    }
                    // CSI <n> B -- moves cursor n rows down (CUD)
                    case 'B':
                        seq.setDefault(0, 1);
                        if (seq.numArgs() != 1)
                            break;
                        LOG(SEQ) << "CUD: setCursor " << cursorPosition().x() << ", " << cursorPosition().y() + seq[0];
                        setCursorPosition(cursorPosition() + Point{0, seq[0]});
                        return;
                    // CSI <n> C -- moves cursor n columns forward (right) (CUF)
                    case 'C':
                        seq.setDefault(0, 1);
                        if (seq.numArgs() != 1)
                            break;
                        LOG(SEQ) << "CUF: setCursor " << cursorPosition().x() + seq[0] << ", " << cursorPosition().y();
                        setCursorPosition(cursorPosition() + Point{seq[0], 0});
                        return;
                    // CSI <n> D -- moves cursor n columns back (left) (CUB)
                    case 'D': {// cursor backward
                        seq.setDefault(0, 1);
                        if (seq.numArgs() != 1)
                            break;
                        int c = cursorPosition().x() >= seq[0] ? cursorPosition().x() - seq[0] : 0;
                        LOG(SEQ) << "CUB: setCursor " << c << ", " << cursorPosition().y();
                        setCursorPosition(Point{c, cursorPosition().y()});
                        return;
                    }
                    /* CSI <n> G -- set cursor character absolute (CHA)
                    */
                    case 'G':
                        seq.setDefault(0, 1);
                        LOG(SEQ) << "CHA: set column " << seq[0] - 1;
                        setCursorPosition(Point{seq[0] - 1, cursorPosition().y()});
                        return;
                    /* set cursor position (CUP) */
                    case 'H': // CUP
                    case 'f': // HVP
                        seq.setDefault(0, 1).setDefault(1, 1);
                        if (seq.numArgs() != 2)
                            break;
                        seq.conditionalReplace(0, 0, 1);
                        seq.conditionalReplace(1, 0, 1);
                        LOG(SEQ) << "CUP: setCursor " << seq[1] - 1 << ", " << seq[0] - 1;
                        setCursorPosition(Point{seq[1] - 1, seq[0] - 1});
                        return;
                    /* CSI <n> J -- erase display, depending on <n>:
                        0 = erase from the current position (inclusive) to the end of display
                        1 = erase from the beginning to the current position(inclusive)
                        2 = erase entire display
                    */
                    case 'J':
                        if (seq.numArgs() > 1)
                            break;
                        switch (seq[0]) {
                            case 0:
                                updateCursorPosition();
                                state_->canvas.fill(
                                    Rect{cursorPosition(), Point{state_->buffer.width(), cursorPosition().y() + 1}},
                                    state_->cell
                                );
                                state_->canvas.fill(
                                    Rect{Point{0, cursorPosition().y() + 1}, Point{state_->buffer.width(), state_->buffer.height()}},                                
                                    state_->cell
                                );
                                return;
                            case 1:
                                updateCursorPosition();
                                state_->canvas.fill(
                                    Rect{Point{}, Point{state_->buffer.width(), cursorPosition().y()}},
                                    state_->cell
                                );
                                state_->canvas.fill(
                                    Rect{Point{0, cursorPosition().y()}, cursorPosition() + Point{1,1}},
                                    state_->cell
                                );
                                return;
                            case 2:
                                state_->canvas.fill(
                                    Rect{state_->buffer.size()},
                                    state_->cell
                                );
                                return;
                            default:
                                break;
                        }
                        break;
                    /* CSI <n> K -- erase in line, depending on <n>
                        0 = Erase to Right
                        1 = Erase to Left
                        2 = Erase entire line
                    */
                    case 'K':
                        if (seq.numArgs() > 1)
                            break;
                        switch (seq[0]) {
                            case 0:
                                updateCursorPosition();
                                state_->canvas.fill(
                                    Rect{cursorPosition(), Point{state_->buffer.width(), cursorPosition().y() + 1}},
                                    state_->cell
                                );
                                return;
                            case 1:
                                updateCursorPosition();
                                state_->canvas.fill(
                                    Rect{Point{0, cursorPosition().y()}, Point{cursorPosition().x() + 1, cursorPosition().y() + 1}},
                                    state_->cell
                                );
                                return;
                            case 2:
                                updateCursorPosition();
                                state_->canvas.fill(
                                    Rect{Point{0, cursorPosition().y()}, Size{state_->buffer.width(),1}},
                                    state_->cell
                                );
                                return;
                            default:
                                break;
                        }
					break;
                    /* CSI <n> L -- Insert n lines. (IL)
                     */
                    case 'L':
                        seq.setDefault(0, 1);
                        LOG(SEQ) << "IL: scrollUp " << seq[0];
                        insertLines(seq[0], cursorPosition().y(), state_->scrollEnd, state_->cell);
                        return;
                    /* CSI <n> M -- Remove n lines. (DL)
                     */
                    case 'M':
                        seq.setDefault(0, 1);
                        LOG(SEQ) << "DL: scrollDown " << seq[0];
                        deleteLines(seq[0], cursorPosition().y(), state_->scrollEnd, state_->cell);
                        return;
                    /* CSI <n> P -- Delete n charcters. (DCH) 
                     */
                    case 'P':
                        seq.setDefault(0, 1);
                        LOG(SEQ) << "DCH: deleteCharacter " << seq[0];
                        deleteCharacters(seq[0]);
                        return;
                    /* CSI <n> S -- Scroll up n lines
                     */
                    case 'S':
                        seq.setDefault(0, 1);
                        LOG(SEQ) << "SU: scrollUp " << seq[0];
                        deleteLines(seq[0], state_->scrollStart, state_->scrollEnd, state_->cell);
                        return;
                    /* CSI <n> T -- Scroll down n lines
                     */
                    case 'T':
                        seq.setDefault(0, 1);
                        LOG(SEQ) << "SD: scrollDown " << seq[0];
                        insertLines(seq[0], cursorPosition().y(), state_->scrollEnd, state_->cell);
                        return;
                    /* CSI <n> X -- erase <n> characters from the current position
                     */
                    case 'X': {
                        seq.setDefault(0, 1);
                        if (seq.numArgs() != 1)
                            break;
                        updateCursorPosition();
                        // erase from first line
                        int n = static_cast<unsigned>(seq[0]);
                        int l = std::min(state_->buffer.width() - cursorPosition().x(), n);
                        state_->canvas.fill(
                            Rect{cursorPosition(), Size{l, 1}},
                            state_->cell
                        );
                        n -= l;
                        // while there is enough stuff left to be larger than a line, erase entire line
                        l = cursorPosition().y() + 1;
                        while (n >= state_->buffer.width() && l < state_->buffer.height()) {
                            state_->canvas.fill(
                                Rect{Point{0,l}, Size{state_->buffer.width(), 1}},
                                state_->cell    
                            );
                            ++l;
                            n -= state_->buffer.width();
                        }
                        // if there is still something to erase, erase from the beginning
                        if (n != 0 && l < state_->buffer.height())
                            state_->canvas.fill(
                                Rect{Point{0, l}, Size{n, 1}},
                                state_->cell
                            );
                        return;
                    }
                    /* CSI <n> b - repeat the previous character n times (REP)
                     */
                    case 'b': {
                        seq.setDefault(0, 1);
                        if (cursorPosition().x() == 0 || cursorPosition().x() + seq[0] >= state_->buffer.width()) {
                            LOG(SEQ_ERROR) << "Repeat previous character out of bounds";
                        } else {
                            LOG(SEQ) << "Repeat previous character " << seq[0] << " times";
                            Cell const & prev = state_->buffer.at(cursorPosition() - Point{1, 0});
                            for (size_t i = 0, e = seq[0]; i < e; ++i) {
                                state_->buffer.at(cursorPosition()) = prev;
                                setCursorPosition(cursorPosition() + Point{1,0});
                            }
                        }
                        return;
                    }
                    /* CSI <n> c - primary device attributes.
                     */
                    case 'c': {
                        if (seq[0] != 0)
                            break;
                        LOG(SEQ) << "Device Attributes - VT102 sent";
                        send("\033[?6c", 5); // send VT-102 for now, go for VT-220? 
                        return;
                    }
                    /* CSI <n> d -- Line position absolute (VPA)
                     */
                    case 'd': {
                        seq.setDefault(0, 1);
                        if (seq.numArgs() != 1)
                            break;
                        int r = seq[0];
                        if (r < 1)
                            r = 1;
                        else if (r > state_->buffer.height())
                            r = state_->buffer.height();
                        LOG(SEQ) << "VPA: setCursor " << cursorPosition().x() << ", " << r - 1;
                        setCursorPosition(Point{cursorPosition().x(), r - 1});
                        return;
                    }
                    /* CSI <n> h -- Reset mode enable
                      
                       Depending on the argument, certain things are turned on. None of the RM settings are currently supported.
                     */
                    case 'h':
                        break;
                    /* CSI <n> l -- Reset mode disable
                    
                       Depending on the argument, certain things are turned off. Turning the features on/off is not allowed, but if the client wishes to disable something that is disabled, it's happily ignored.
                     */
                    case 'l':
                        seq.setDefault(0, 0);
                        // enable replace mode (IRM) since this is the only mode we allow, do nothing
                        if (seq[0] == 4)
                            return;
                        // powershell is sending CSI 25 l which means nothing and likely is a bug, perhaps should be CSI ? 25 l to disable cursor? 
                        break;
                    /* SGR
                     */
                    case 'm':
                        return parseSGR(seq);
                    /** Device Status Report - DSR
                     */
                    case 'n':
                        // status report, send CSI 0 n which means OK
                        if (seq[0] == 5) {
                            send("\033[0n", 4);
                        // cursor position, send CSI row ; col R
                        } else if (seq[0] == 6) {
                            std::string cpos = STR("\033[" << (cursorPosition().y() + 1) << ";" << (cursorPosition().x() + 1) << "R");
                            send(cpos.c_str(), cpos.size());
                        // invalid DSR code
                        } else {
                            break;
                        }
                        return;
                    /* CSI <n> ; <n> r -- Set scrolling region (default is the whole window) (DECSTBM)
                     */
                    case 'r':
                        seq.setDefault(0, 1); // inclusive
                        seq.setDefault(1, cursorPosition().y()); // inclusive
                        if (seq.numArgs() != 2)
                            break;
                        // This is not proper 
                        seq.conditionalReplace(0, 0, 1);
                        seq.conditionalReplace(1, 0, 1);
                        if (seq[0] > state_->buffer.height())
                            break;
                        if (seq[1] > state_->buffer.height())
                            break;
                        state_->scrollStart = std::min(seq[0] - 1, state_->buffer.height() - 1); // inclusive
                        state_->scrollEnd = std::min(seq[1], state_->buffer.height()); // exclusive 
                        setCursorPosition(Point{0,0});
                        LOG(SEQ) << "Scroll region set to " << state_->scrollStart << " - " << state_->scrollEnd;
                        return;
                    /* CSI <n> : <n> : <n> t -- window manipulation (xterm)

                        We do nothing for these at the moment, just recognize the few possibly interesting ones.
                     */
                    case 't':
                        seq.setDefault(0, 0).setDefault(1, 0).setDefault(2, 0);
                        switch (seq[0]) {
                        case 22:
                            // 22;0;0 -- save xterm icon and window title on stack
                            if (seq[1] == 0 && seq[2] == 0)
                                return;
                            break;
                        case 23:
                            // 23;0;0 -- restore xterm icon and window title from stack
                            if (seq[1] == 0 && seq[2] == 0)
                                return;
                            break;
                        default:
                            break;
                        }
                        break;
                    default:
                        break;
                }
                break;
            // getters and setters
            case '?':
                switch (seq.finalByte()) {
                    case 'h':
                        return parseCSIGetterOrSetter(seq, true);
                    case 'l':
                        return parseCSIGetterOrSetter(seq, false);
                    case 's':
                    case 'r':
                        return parseCSISaveOrRestore(seq);
                    default:
                        break;
                }
                break;
            // other CSI sequences
            case '>':
                switch (seq.finalByte()) {
    				/* CSI > 0 c -- Send secondary device attributes.
                     */
                    case 'c':
                        if (seq[0] != 0)
                            break;
					LOG(SEQ) << "Secondary Device Attributes - VT100 sent";
					send("\033[>0;0;0c", 9); // we are VT100, no version third must always be zero (ROM cartridge)
					return;
				default:
					break;
                }
                break;
            default:
                break;
        }
		LOG(SEQ_UNKNOWN) << " Unknown CSI sequence " << seq;
    }

    void AnsiTerminal::parseCSIGetterOrSetter(CSISequence & seq, bool value) {
		for (size_t i = 0; i < seq.numArgs(); ++i) {
			int id = seq[i];
			switch (id) {
				/* application cursor mode on/off
				 */
				case 1:
					cursorMode_ = value ? CursorMode::Application : CursorMode::Normal;
					LOG(SEQ) << "application cursor mode: " << value;
					continue;
				/* Smooth scrolling -- ignored*/
				case 4:
					LOG(SEQ_WONT_SUPPORT) << "Smooth scrolling: " << value;
					continue;
				/* DECAWM - autowrap mode on/off */
				case 7:
					if (value) {
						LOG(SEQ) << "autowrap mode enable (by default)";
                    } else {
						LOG(SEQ_UNKNOWN) << "CSI?7l, DECAWM does not support being disabled";
                    }
					continue;
				// cursor blinking
				case 12:
                    cursor().setBlink(value);
					LOG(SEQ) << "cursor blinking: " << value;
					continue;
				// cursor show/hide
				case 25:
                    cursor().setVisible(value);
					LOG(SEQ) << "cursor visible: " << value;
					continue;
				/* Mouse tracking movement & buttons.

				https://stackoverflow.com/questions/5966903/how-to-get-mousemove-and-mouseclick-in-bash
				*/
				/* Enable normal mouse mode, i.e. report button press & release events only.
				 */
				case 1000:
                    mouseMode_ = value ? MouseMode::Normal : MouseMode::Off;
					LOG(SEQ) << "normal mouse tracking: " << value;
					continue;
				/* Mouse highlighting - will not support because it requires supporting application and may hang terminal if not used properly, which sounds rather dangerous.
				 */
				case 1001:
					LOG(SEQ_WONT_SUPPORT) << "hilite mouse mode";
					continue;
				/* Mouse button events (report mouse button press & release and mouse movement if any of the buttons is down.
				 */
				case 1002:
                    mouseMode_ = value ? MouseMode::ButtonEvent : MouseMode::Off;
					LOG(SEQ) << "button-event mouse tracking: " << value;
					continue;
				/* Report all mouse events (i.e. report mouse move even when buttons are not pressed).
				 */
				case 1003:
                    mouseMode_ = value ? MouseMode::All : MouseMode::Off;
					LOG(SEQ) << "all mouse tracking: " << value;
					continue;
				/* UTF8 encoded tracking.
				 */
				case 1005:
					//mouseEncoding_ = value ? MouseEncoding::UTF8 : MouseEncoding::Default;
					LOG(SEQ_WONT_SUPPORT) << "UTF8 mouse encoding: " << value;
					continue;
				/* SGR mouse encoding.
				 */
				case 1006: // 
					mouseEncoding_ = value ? MouseEncoding::SGR : MouseEncoding::Default;
					LOG(SEQ) << "UTF8 mouse encoding: " << value;
					continue;
				/* Enable or disable the alternate screen buffer.
				 */
				case 47:
				case 1049: 
                    if (alternateMode_ != value) {
                        // if the selection update was in progress, cancel it. If the selection is not empty, clear it - this has to be an UI event as clearSelection is UI action
                        schedule([this](){
                            cancelSelectionUpdate();
                            clearSelection();
                        });
                        // perform the mode change
                        std::swap(state_, stateBackup_);
                        alternateMode_ = value;
                        schedule([this](){
                            if (alternateMode_)
                                setScrollOffset(Point{0, 0});
                            else
                                setScrollOffset(Point{0, static_cast<int>(historyRows_.size())});
                        });
                        // if we are entering the alternate mode, reset the state to default values
                        if (value) {
                            state_->reset(palette_->defaultForeground(), palette_->defaultBackground());
                            state_->invalidateLastCharacter();
                            LOG(SEQ) << "Alternate mode on";
                        } else {
                            LOG(SEQ) << "Alternate mode off";
                        }
                    }
					continue;
				/* Enable/disable bracketed paste mode. When enabled, if user pastes code in the window, the contents should be enclosed with ESC [200~ and ESC[201~ so that the client app can determine it is contents of the clipboard (things like vi might otherwise want to interpret it. 
				 */
				case 2004:
					bracketedPaste_ = value;
					continue;
				default:
					break;
			}
			LOG(SEQ_UNKNOWN) << "Invalid Get/Set command: " << seq;
		}
    }

    void AnsiTerminal::parseCSISaveOrRestore(CSISequence & seq) {
		for (size_t i = 0; i < seq.numArgs(); ++i)
			LOG(SEQ_WONT_SUPPORT) << "Private mode " << (seq.finalByte() == 's' ? "save" : "restore") << ", id " << seq[i];
    }

    void AnsiTerminal::parseSGR(CSISequence & seq) {
        seq.setDefault(0, 0);
		for (size_t i = 0; i < seq.numArgs(); ++i) {
			switch (seq[i]) {
				/* Resets all attributes. */
				case 0:
                    state_->cell.setFg(palette_->defaultForeground()) 
                               .setDecor(palette_->defaultForeground()) 
                               .setBg(palette_->defaultBackground())
                               .setFont(Font{});
                    state_->inverseMode = false;
                    LOG(SEQ) << "font fg bg reset";
					break;
				/* Bold / bright foreground. */
				case 1:
					state_->cell.font().setBold();
					LOG(SEQ) << "bold set";
					break;
				/* faint font (light) - won't support for now, though in theory we easily can. */
				case 2:
					LOG(SEQ_WONT_SUPPORT) << "faint font";
					break;
				/* Italic */
				case 3:
					state_->cell.font().setItalic();
					LOG(SEQ) << "italics set";
					break;
				/* Underline */
				case 4:
                    state_->cell.font().setUnderline();
					LOG(SEQ) << "underline set";
					break;
				/* Blinking text */
				case 5:
                    state_->cell.font().setBlink();
					LOG(SEQ) << "blink set";
					break;
				/* Inverse on */
				case 7:
                    if (! state_->inverseMode) {
                        state_->inverseMode = true;
                        Color fg = state_->cell.fg();
                        Color bg = state_->cell.bg();
                        state_->cell.setFg(bg).setDecor(bg).setBg(fg);
    					LOG(SEQ) << "inverse mode on";
                    }
                    break;
				/* Strikethrough */
				case 9:
                    state_->cell.font().setStrikethrough();
					LOG(SEQ) << "strikethrough";
					break;
				/* Bold off */
				case 21:
					state_->cell.font().setBold(false);
					LOG(SEQ) << "bold off";
					break;
				/* Normal - neither bold, nor faint. */
				case 22:
					state_->cell.font().setBold(false).setItalic(false);
					LOG(SEQ) << "normal font set";
					break;
				/* Italic off. */
				case 23:
					state_->cell.font().setItalic(false);
					LOG(SEQ) << "italic off";
					break;
				/* Disable underline. */
				case 24:
                    state_->cell.font().setUnderline(false);
					LOG(SEQ) << "undeline off";
					break;
				/* Disable blinking. */
				case 25:
                    state_->cell.font().setBlink(false);
					LOG(SEQ) << "blink off";
					break;
                /* Inverse off */
				case 27: 
                    if (state_->inverseMode) {
                        state_->inverseMode = false;
                        Color fg = state_->cell.fg();
                        Color bg = state_->cell.bg();
                        state_->cell.setFg(bg).setDecor(bg).setBg(fg);
    					LOG(SEQ) << "inverse mode off";
                    }
                    break;
				/* Disable strikethrough. */
				case 29:
                    state_->cell.font().setStrikethrough(false);
					LOG(SEQ) << "Strikethrough off";
					break;
				/* 30 - 37 are dark foreground colors, handled in the default case. */
				/* 38 - extended foreground color */
				case 38: {
                    Color fg = parseSGRExtendedColor(seq, i);
                    state_->cell.setFg(fg).setDecor(fg);    
					LOG(SEQ) << "fg set to " << fg;
					break;
                }
				/* Foreground default. */
				case 39:
                    state_->cell.setFg(palette_->defaultForeground())
                               .setDecor(palette_->defaultForeground());
					LOG(SEQ) << "fg reset";
					break;
				/* 40 - 47 are dark background color, handled in the default case. */
				/* 48 - extended background color */
				case 48: {
                    Color bg = parseSGRExtendedColor(seq, i);
                    state_->cell.setBg(bg);    
					LOG(SEQ) << "bg set to " << bg;
					break;
                }
				/* Background default */
				case 49:
					state_->cell.setBg(palette_->defaultBackground());
					LOG(SEQ) << "bg reset";
					break;
				/* 90 - 97 are bright foreground colors, handled in the default case. */
				/* 100 - 107 are bright background colors, handled in the default case. */
				default:
					if (seq[i] >= 30 && seq[i] <= 37) {
                        int colorIndex = seq[i] - 30;
                        if (boldIsBright_ && state_->cell.font().bold())
                            colorIndex += 8;
						state_->cell.setFg(palette_->at(colorIndex))
                                   .setDecor(palette_->at(colorIndex));
						LOG(SEQ) << "fg set to " << palette_->at(seq[i] - 30);
					} else if (seq[i] >= 40 && seq[i] <= 47) {
						state_->cell.setBg(palette_->at(seq[i] - 40));
						LOG(SEQ) << "bg set to " << palette_->at(seq[i] - 40);
					} else if (seq[i] >= 90 && seq[i] <= 97) {
						state_->cell.setFg(palette_->at(seq[i] - 82))
                                   .setDecor(palette_->at(seq[i] - 82));
						LOG(SEQ) << "fg set to " << palette_->at(seq[i] - 82);
					} else if (seq[i] >= 100 && seq[i] <= 107) {
						state_->cell.setBg(palette_->at(seq[i] - 92));
						LOG(SEQ) << "bg set to " << palette_->at(seq[i] - 92);
					} else {
						LOG(SEQ_UNKNOWN) << "Invalid SGR code: " << seq;
					}
					break;
			}
		}
    }

    Color AnsiTerminal::parseSGRExtendedColor(CSISequence & seq, size_t & i) {
		++i;
		if (i < seq.numArgs()) {
			switch (seq[i++]) {
				/* index from 256 colors */
				case 5:
					if (i >= seq.numArgs()) // not enough args 
						break;
					if (seq[i] > 255) // invalid color spec
						break;
                    return palette_->at(seq[i]);
				/* true color rgb */
				case 2:
					i += 2;
					if (i >= seq.numArgs()) // not enough args
						break;
					if (seq[i - 2] > 255 || seq[i - 1] > 255 || seq[i] > 255) // invalid color spec
						break;
					return Color(seq[i - 2] & 0xff, seq[i - 1] & 0xff, seq[i] & 0xff);
				/* everything else is an error */
				default:
					break;
			}
		}
		LOG(SEQ_UNKNOWN) << "Invalid extended color: " << seq;
		return Color::White;
    }

    void AnsiTerminal::parseOSCSequence(OSCSequence & seq) {
        switch (seq.num()) {
            /* OSC 0 - change window title and icon name
               OSC 2 - change window title
             */
            case 0:
            case 2: {
    			LOG(SEQ) << "Title change to " << seq.value();
                schedule([this, title = seq.value()](){
                    StringEvent::Payload p(title);
                    onTitleChange(p, this);
                });
                return;
            }
            /* OSC 1 - change icon name 
               
               The icon name is a shortened text that should be displayed next to the iconified window. From: https://unix.stackexchange.com/questions/265760/what-does-it-mean-to-set-a-terminals-icon-title
             */
            case 1: {
                // TODO do we want to support this? Not ATM...
                return;
            }
            /* OSC 52 - set clipboard to given value. 
             */
            case 52: {
                if (StartsWith(seq.value(), "c;")) {
                    std::string text = seq.value().substr(2);
                    LOG(SEQ) << "Clipboard set to " << text;
                    schedule([this, contents = text]() {
                        StringEvent::Payload p{contents};
                        onClipboardSetRequest(p, this);
                    });
                    return;
                }
                break;
            }
            /* OSC 112 - reset cursor color. 
             */
            case 112:
                LOG(SEQ) << "Cursor color reset";
                cursor().setColor(defaultCursor_.color());
                return;
            default:
                break;
        }
        LOG(SEQ_UNKNOWN) << "Invalid OSC sequence: " << seq;
    }



    // ============================================================================================
    // AnsiTerminal::Buffer

    void AnsiTerminal::Buffer::insertLine(int top, int bottom, Cell const & fill) {
        Cell * x = rows_[bottom - 1];
        memmove(rows_ + top + 1, rows_ + top, sizeof(Cell*) * (bottom - top - 1));
        rows_[top] = x;
        fillRow(top, fill, 0, width());
    }

    std::pair<AnsiTerminal::Cell *, int> AnsiTerminal::Buffer::copyRow(int row, Color defaultBg) {
        int lastCol = width();
        Cell * x = rows_[row];
        while (lastCol-- > 0) {
            Cell & c = x[lastCol];
            // if we have found end of line character, good
            if (IsLineEnd(c))
                break;
            // if we have found a visible character, we must remember the whole line, break the search - any end of line characters left of it will make no difference
            if (c.codepoint() != ' ' || c.bg() != defaultBg || c.font().underline() || c.font().strikethrough()) {
                break;
            }
        }   
        // if we are not at the end of line, we must remember the whole line
        if (IsLineEnd(x[lastCol])) 
            lastCol += 1;
        else
            lastCol = width();
        // make the copy and return it
        Cell * result = new Cell[lastCol];
        memcpy(result, x, sizeof(Cell) * lastCol);
        return std::make_pair(result, lastCol);
    }

    void AnsiTerminal::Buffer::deleteLine(int top, int bottom, Cell const & fill) {
        Cell * x = rows_[top];
        memmove(rows_ + top, rows_ + top + 1, sizeof(Cell*) * (bottom - top - 1));
        rows_[bottom - 1] = x;
        fillRow(bottom - 1, fill, 0, width());
    }

    void AnsiTerminal::Buffer::resize(Size size, Cell const & fill, std::function<void(Cell*, int)> addToHistory) {
        if (size_ == size)
            return;
        // create backup of the cells and new cells
        Cell ** oldRows = rows_;
        int oldWidth = this->width();
        int oldHeight = this->height();
        rows_ = nullptr;
        // resize the contents (create new ones) and fill it with the specified cell
        Canvas::Buffer::resize(size);
        this->fill(fill);
		// now determine the row at which we should stop - this is done by going back from cursor's position until we hit end of line, that would be the last line we will use
		int stopRow = cursorPosition_.y() - 1;
		while (stopRow >= 0) {
			Cell* row = oldRows[stopRow];
			int i = 0;
			for (; i < oldWidth; ++i)
                if (IsLineEnd(row[i]))
					break;
			// we have found the line end
			if (i < oldWidth) {
				++stopRow; // stop after current row
				break;
			}
			// otherwise try the above line
			--stopRow;
		}
        if (stopRow < 0)
            stopRow = 0;
        // now transfer the contents, moving any lines that won't fit into the terminal will be moved to the history 
		int oldCursorRow = cursorPosition_.y();
		cursorPosition_ = Point{0,0};
		for (int y = 0; y < stopRow; ++y) {
			for (int x = 0; x < oldWidth; ++x) {
				Cell& cell = oldRows[y][x];
				rows_[cursorPosition_.y()][cursorPosition_.x()] = cell;
				// if the copied cell is end of line, or if we are at the end of new line, increase the cursor row
				if (IsLineEnd(cell) || (cursorPosition_ += Point{1,0}).x() == width())
                    cursorPosition_ += Point{-cursorPosition_.x(),1};
				// scroll the new lines if necessary
				if (cursorPosition_.y() == height()) {
                    // add the line to history, first determine the last column, then create a copy and call the inserter
                    if (addToHistory) {
                        Cell * row = rows_[0];
                        int lastCol = width();
                        while (lastCol-- > 0) {
                            Cell & c = row[lastCol];
                            // if we have found end of line character, good
                            if (IsLineEnd(c))
                                break;
                            // if we have found a visible character, we must remember the whole line, break the search - any end of line characters left of it will make no difference
                            if (c.codepoint() != ' ' || c.bg() != fill.bg() || c.font().underline() || c.font().strikethrough()) 
                                break;
                        }
                        // if we are not at the end of line, we must remember the whole line
                        if (IsLineEnd(row[lastCol])) 
                            lastCol += 1;
                        else
                            lastCol = width();
                        // make the copy and add it to the history line
                        Cell * rowCopy = new Cell[lastCol];
                        memcpy(rowCopy, row, sizeof(Cell) * lastCol);
                        addToHistory(rowCopy, lastCol);
                    }
                    deleteLine(0, height(), fill);
                    cursorPosition_ -= Point{0,1};
				}
				// if it was new line, skip whatever was afterwards
                if (IsLineEnd(cell))
					break;
			}
		}
		// the contents was transferred, delete the old cells
        for (int i = 0; i < oldHeight; ++i)
            delete [] oldRows[i];
        delete [] oldRows;
		// because the first thing the app will do after resize is to adjust the cursor position if the current line span more than 1 terminal line, we must account for this and update cursor position
		cursorPosition_ += Point{0, oldCursorRow - stopRow};
    }

    // ============================================================================================
    // AnsiTerminal::Palette

    AnsiTerminal::Palette AnsiTerminal::Palette::Colors16() {
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

    AnsiTerminal::Palette AnsiTerminal::Palette::XTerm256() {
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

    AnsiTerminal::Palette::Palette(std::initializer_list<Color> colors, Color defaultFg, Color defaultBg):
        size_(colors.size()),
        defaultFg_(defaultFg),
        defaultBg_(defaultBg),
        colors_(new Color[colors.size()]) {
		unsigned i = 0;
		for (Color c : colors)
			colors_[i++] = c;
    }

    AnsiTerminal::Palette::Palette(Palette const & from):
        size_(from.size_),
        defaultFg_(from.defaultFg_),
        defaultBg_(from.defaultBg_),
        colors_(new Color[from.size_]) {
		memcpy(colors_, from.colors_, sizeof(Color) * size_);
    }

    AnsiTerminal::Palette::Palette(Palette && from):
        size_(from.size_),
        defaultFg_(from.defaultFg_),
        defaultBg_(from.defaultBg_),
        colors_(from.colors_) {
        from.colors_ = nullptr;
        from.size_ = 0;
    }

    AnsiTerminal::Palette & AnsiTerminal::Palette::operator = (AnsiTerminal::Palette const & other) {
        if (&other != this) {
            delete [] colors_;
            size_ = other.size_;
            defaultFg_ = other.defaultFg_;
            defaultBg_ = other.defaultBg_;
            colors_ = new Color[size_];
    		memcpy(colors_, other.colors_, sizeof(Color) * size_);
        }
        return *this;
    }

    AnsiTerminal::Palette & AnsiTerminal::Palette::operator == (AnsiTerminal::Palette && other) {
        if (&other != this) {
            delete [] colors_;
            size_ = other.size_;
            defaultFg_ = other.defaultFg_;
            defaultBg_ = other.defaultBg_;
            colors_ = other.colors_;
            other.colors_ = nullptr;
            other.size_ = 0;
        }
        return *this;
    }

} // namespace ui
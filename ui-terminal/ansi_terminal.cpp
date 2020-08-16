#include "ansi_terminal.h"

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
        }
    }

    std::unordered_map<Key, std::string> AnsiTerminal::KeyMap_;
    std::unordered_set<Key> AnsiTerminal::PrintableKeys_;


    // TODO remove the underscores
    Log AnsiTerminal::SEQ("VT100");
    Log AnsiTerminal::SEQ_UNKNOWN("VT100_UNKNOWN");
    Log AnsiTerminal::SEQ_ERROR("VT100_ERROR");
    Log AnsiTerminal::SEQ_WONT_SUPPORT("VT100_WONT_SUPPORT");
    Log AnsiTerminal::SEQ_SENT("VT100_SENT");


    char32_t AnsiTerminal::LineDrawingChars_[15] = {0x2518, 0x2510, 0x250c, 0x2514, 0x253c, 0, 0, 0x2500, 0, 0, 0x251c, 0x2524, 0x2534, 0x252c, 0x2502};

    AnsiTerminal::AnsiTerminal(tpp::PTYMaster * pty, Palette * palette):
        PTYBuffer{pty},
        palette_{palette},
        state_{new State{}},
        stateBackup_{new State{}} {
        if (KeyMap_.empty()) {
            InitializeKeyMap(KeyMap_);
            InitializePrintableKeys(PrintableKeys_);
        }
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
        std::lock_guard<PriorityLock> g(bufferLock_.priorityLock(), std::adopt_lock);
#ifdef SHOW_LINE_ENDINGS
        Border endOfLine{Border{Color::Red}.setAll(Border::Kind::Thin)};
#endif
        Rect visibleRect{canvas.visibleRect()};
        int top = historyRows();
        // see if there are any history lines that need to be drawn
        for (int row = std::max(0, visibleRect.top()), re = std::min(top, visibleRect.bottom()); ; ++row) {
            if (row >= re)
                break;
            for (int col = 0, ce = historyRows_[row].first; col < ce; ++col) {
                canvas.at(Point{col, row}) = historyRows_[row].second[col];
#ifdef SHOW_LINE_ENDINGS
                if (isEndOfLine(history_[row].second[col]))
                    canvas.setBorderAt(Point{col, row}, endOfLine);
#endif
            }
            canvas.fill(Rect{Point{historyRows_[row].first, row}, Point{width(), row + 1}});
        }
        // now draw the actual buffer
        canvas.drawBuffer(state_->buffer, Point{0, top});
#ifdef  SHOW_LINE_ENDINGS
        // now add borders to the cells that are marked as end of line
        for (int row = std::max(top, visibleRect.top()), re = visibleRect.bottom(); ; ++row) {
            if (row >= re)
                break;
            for (int col = 0; col < width(); ++col) {
                if (isEndOfLine(at(col, row - top)))
                    canvas.at(Point{col, row}).setBorder(endOfLine);
            }
        }
#endif
    }

    // Inputs

    void AnsiTerminal::keyDown(KeyEvent::Payload & e) {
        // only scroll to prompt if the key down is not a simple modifier key
        if (*e != Key::Shift + Key::ShiftKey && *e != Key::Alt + Key::AltKey && *e != Key::Ctrl + Key::CtrlKey && *e != Key::Win + Key::WinKey)
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
        // don't propagate to parent as the terminal handles keyboard input itself
        e.stop();
        Widget::keyDown(e);
    }

    void AnsiTerminal::keyUp(KeyEvent::Payload & e) {
        e.stop();
        Widget::keyUp(e);
    }

    void AnsiTerminal::keyChar(KeyCharEvent::Payload & e) {
        ASSERT(e->codepoint() >= 32);
        send(e->toCharPtr(), e->size());
        // don't propagate to parent as the terminal handles keyboard input itself
        e.stop();
        Widget::keyChar(e);

    }



    // Terminal State 

    void AnsiTerminal::deleteCharacters(unsigned num) {
		int r = state_->cursor.y();
		for (unsigned c = state_->cursor.x(), e = state_->buffer.width() - num; c < e; ++c) 
			state_->buffer.at(c, r) = state_->buffer.at(c + num, r);
		for (unsigned c = state_->buffer.width() - num, e = state_->buffer.width(); c < e; ++c)
			state_->buffer.at(c, r) = state_->cell;
    }

    void AnsiTerminal::insertCharacters(unsigned num) {
		unsigned r = state_->cursor.y();
		// first copy the characters
		for (unsigned c = state_->buffer.width() - 1, e = state_->cursor.x() + num; c >= e; --c)
			state_->buffer.at(c, r) = state_->buffer.at(c - num, r);
		for (unsigned c = state_->cursor.x(), e = state_->cursor.x() + num; c < e; ++c)
			state_->buffer.at(c, r) = state_->cell;
    }

    void AnsiTerminal::updateCursorPosition() {
        while (state_->cursor.x() >= state_->buffer.width()) {
            ASSERT(state_->buffer.width() > 0);
            state_->cursor -= Point{state_->buffer.width(), -1};
            // if the cursor is on the last line, evict the lines above
            if (state_->cursor.y() == state_->scrollEnd) 
                deleteLines(1, state_->scrollStart, state_->scrollEnd, state_->cell);
        }
        if (state_->cursor.y() >= state_->buffer.height())
            state_->cursor.setY(state_->buffer.height() - 1);
        // the cursor position must be valid now
        ASSERT(state_->cursor.x() < state_->buffer.width());
        ASSERT(state_->cursor.y() < state_->buffer.height());
        // set last character position to the now definitely valid cursor coordinates
        state_->lastCharacter_ = state_->cursor;
    }

    // Scrollback buffer

    void AnsiTerminal::insertLines(int lines, int top, int bottom, Cell const & fill) {
        state_->buffer.insertLines(lines, top, bottom, fill);
    }

    void AnsiTerminal::deleteLines(int lines, int top, int bottom, Cell const & fill) {
        if (top == 0 && scrollOffset().y() == historyRows()) {
            state_->buffer.deleteLines(lines, top, bottom, fill, palette_->defaultBackground());
            // if the window was scrolled to the end, keep it scrolled to the end as well
            // this means that when the scroll buffer overflows, the scroll offset won't change, but its contents would
            // for now I think this is a feature as you then know that your scroll buffer is overflowing
            setScrollOffset(Point{0, historyRows()});
        } else {
            state_->buffer.deleteLines(lines, top, bottom, fill, palette_->defaultBackground());
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
        //Cell & cell = state_->buffer.set(state_->cursor, state_->cell, codepoint);
        Cell & cell = state_->buffer.at(state_->cursor);
        cell = state_->cell;
        cell.setCodepoint(codepoint);
        // advance cursor's column
        state_->cursor += Point{1, 0};

        // what's left is to deal with corner cases, such as larger fonts & double width characters
        int columnWidth = Char::ColumnWidth(codepoint);

        // if the character's column width is 2 and current font is not double width, update to double width font
        if (columnWidth == 2 && ! cell.font().doubleWidth()) {
            //columnWidth = 1;
            cell.setFont(cell.font().setDoubleWidth(true));
        }

        // TODO do double width & height characters properly for the per-line 

        /*
        // if the character's column width is 2 and current font is not double width, update to double width font
        // if the font's size is greater than 1, copy the character as required (if we are at the top row of double height characters, increase the size artificially)
        int charWidth = state_->doubleHeightTopLine ? cell.font().width() * 2 : cell.font().width();

        while (columnWidth > 0 && state_->cursor.x() < state_->buffer.width()) {
            for (int i = 1; (i < charWidth) && state_->cursor.x() < state_->buffer.width(); ++i) {
                Cell& cell2 = buffer_.at(buffer_.cursor().pos);
                // copy current cell properties
                cell2 = cell;
                // make sure the cell's font is normal size and width and display a space
                cell2.setCodepoint(' ').setFont(cell.font().setSize(1).setDoubleWidth(false));
                ++state_->cursor.x();
            } 
            if (--columnWidth > 0 && state_->cursor.x() < state_->buffer.width()) {
                Cell& cell2 = buffer_.at(buffer_.cursor().pos);
                // copy current cell properties
                cell2 = cell;
                cell2.setCodepoint(' ');
                ++state_->cursor.x();
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
        if (state_->cursor.x() % 8 == 0)
            state_->cursor += Point{8, 0};
        else
            state_->cursor += Point{8 - state_->cursor.x() % 8, 0};
        LOG(SEQ) << "Tab: cursor col is " << state_->cursor.x();
    }

    void AnsiTerminal::parseLF() {
        LOG(SEQ) << "LF";
        state_->markLineEnd();
        // disable double width and height chars
        state_->cell.setFont(state_->cell.font().setSize(1).setDoubleWidth(false));
        state_->cursor += Point{0, 1};
        // determine if region should be scrolled
        if (state_->cursor.y() == state_->scrollEnd) {
            deleteLines(1, state_->scrollStart, state_->scrollEnd, state_->cell);
            state_->cursor -= Point{0, 1};
        }
        // update the cursor position as LF takes immediate effect
        updateCursorPosition();
    }

    void AnsiTerminal::parseCR() {
        LOG(SEQ) << "CR";
        // mark the last character as line end? 
        // TODO
        state_->cursor.setX(0);
    }

    void AnsiTerminal::parseBackspace() {
        LOG(SEQ) << "BACKSPACE";
        if (state_->cursor.x() == 0) {
            if (state_->cursor.y() > 0)
                state_->cursor -= Point{0, 1};
            state_->cursor.setX(state_->buffer.size().width() - 1);
        } else {
            state_->cursor -= Point{1, 0};
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
				if (state_->cursor.y() == state_->scrollStart) 
					insertLines(1, state_->scrollStart, state_->scrollEnd, state_->cell);
				else
                    state_->setCursor(state_->cursor.x(), state_->cursor.y() - 1);
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
        // TODO reenable tpp sequences
        //processTppSequence(TppSequenceEvent{kind, i, tppEnd});
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
                        unsigned r = state_->cursor.y() >= seq[0] ? state_->cursor.y() - seq[0] : 0;
                        LOG(SEQ) << "CUU: setCursor " << state_->cursor.x() << ", " << r;
                        state_->setCursor(state_->cursor.x(), r);
                        return;
                    }
                    // CSI <n> B -- moves cursor n rows down (CUD)
                    case 'B':
                        seq.setDefault(0, 1);
                        if (seq.numArgs() != 1)
                            break;
                        LOG(SEQ) << "CUD: setCursor " << state_->cursor.x() << ", " << state_->cursor.y() + seq[0];
                        state_->setCursor(state_->cursor.x(), state_->cursor.y() + seq[0]);
                        return;
                    // CSI <n> C -- moves cursor n columns forward (right) (CUF)
                    case 'C':
                        seq.setDefault(0, 1);
                        if (seq.numArgs() != 1)
                            break;
                        LOG(SEQ) << "CUF: setCursor " << state_->cursor.x() + seq[0] << ", " << state_->cursor.y();
                        state_->setCursor(state_->cursor.x() + seq[0], state_->cursor.y());
                        return;
                    // CSI <n> D -- moves cursor n columns back (left) (CUB)
                    case 'D': {// cursor backward
                        seq.setDefault(0, 1);
                        if (seq.numArgs() != 1)
                            break;
                        unsigned c = state_->cursor.x() >= seq[0] ? state_->cursor.x() - seq[0] : 0;
                        LOG(SEQ) << "CUB: setCursor " << c << ", " << state_->cursor.y();
                        state_->setCursor(c, state_->cursor.y());
                        return;
                    }
                    /* CSI <n> G -- set cursor character absolute (CHA)
                    */
                    case 'G':
                        seq.setDefault(0, 1);
                        LOG(SEQ) << "CHA: set column " << seq[0] - 1;
                        state_->setCursor(seq[0] - 1, state_->cursor.y());
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
                        state_->setCursor(seq[1] - 1, seq[0] - 1);
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
                                    Rect{state_->cursor, Point{state_->buffer.width(), state_->cursor.y() + 1}},
                                    state_->cell
                                );
                                state_->canvas.fill(
                                    Rect{Point{0, state_->cursor.y() + 1}, Point{state_->buffer.width(), state_->buffer.height()}},                                
                                    state_->cell
                                );
                                return;
                            case 1:
                                updateCursorPosition();
                                state_->canvas.fill(
                                    Rect{Point{}, Point{state_->buffer.width(), state_->cursor.y()}},
                                    state_->cell
                                );
                                state_->canvas.fill(
                                    Rect{Point{0, state_->cursor.y()}, state_->cursor + Point{1,1}},
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
                                    Rect{state_->cursor, Point{state_->buffer.width(), state_->cursor.y() + 1}},
                                    state_->cell
                                );
                                return;
                            case 1:
                                updateCursorPosition();
                                state_->canvas.fill(
                                    Rect{Point{0, state_->cursor.y()}, Point{state_->cursor.x() + 1, state_->cursor.y() + 1}},
                                    state_->cell
                                );
                                return;
                            case 2:
                                updateCursorPosition();
                                state_->canvas.fill(
                                    Rect{Point{0, state_->cursor.y()}, Size{state_->buffer.width(),1}},
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
                        insertLines(seq[0], state_->cursor.y(), state_->scrollEnd, state_->cell);
                        return;
                    /* CSI <n> M -- Remove n lines. (DL)
                     */
                    case 'M':
                        seq.setDefault(0, 1);
                        LOG(SEQ) << "DL: scrollDown " << seq[0];
                        deleteLines(seq[0], state_->cursor.y(), state_->scrollEnd, state_->cell);
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
                        insertLines(seq[0], state_->cursor.y(), state_->scrollEnd, state_->cell);
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
                        int l = std::min(state_->buffer.width() - state_->cursor.x(), n);
                        state_->canvas.fill(
                            Rect{state_->cursor, Size{l, 1}},
                            state_->cell
                        );
                        n -= l;
                        // while there is enough stuff left to be larger than a line, erase entire line
                        l = state_->cursor.y() + 1;
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
                        if (state_->cursor.x() == 0 || state_->cursor.x() + seq[0] >= state_->buffer.width()) {
                            LOG(SEQ_ERROR) << "Repeat previous character out of bounds";
                        } else {
                            LOG(SEQ) << "Repeat previous character " << seq[0] << " times";
                            Cell const & prev = state_->buffer.at(state_->cursor - Point{1, 0});
                            for (size_t i = 0, e = seq[0]; i < e; ++i) {
                                state_->buffer.at(state_->cursor) = prev;
                                state_->cursor += Point{1,0};
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
                        LOG(SEQ) << "VPA: setCursor " << state_->cursor.x() << ", " << r - 1;
                        state_->setCursor(state_->cursor.x(), r - 1);
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
                        break;
                    /* SGR
                     */
                    case 'm':
                        return parseSGR(seq);
                    /* CSI <n> ; <n> r -- Set scrolling region (default is the whole window) (DECSTBM)
                     */
                    case 'r':
                        seq.setDefault(0, 1); // inclusive
                        seq.setDefault(1, state_->cursor.y()); // inclusive
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
                        state_->setCursor(0, 0);
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
                    cursor_.setBlink(value);
					LOG(SEQ) << "cursor blinking: " << value;
					continue;
				// cursor show/hide
				case 25:
                    cursor_.setVisible(value);
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
                        /* TODO re-enable this when we deal with selection
                        schedule([this](){
                            cancelSelectionUpdate();
                            clearSelection();
                        });
                        */
                        // perform the mode change
                        std::swap(state_, stateBackup_);
                        alternateMode_ = value;
                        // TODO renable
                        //setScrollSize(Size{width(), state_->buffer.historyRows() + height()});
                        //setScrollOffset(Point{0, state_->buffer.historyRows()});
                        // if we are entering the alternate mode, reset the state to default values
                        if (value) {
                            state_->reset(palette_->defaultForeground(), palette_->defaultBackground());
                            state_->lastCharacter_ = Point{-1,-1};
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
					state_->cell.setFont(state_->cell.font().setBold());
					LOG(SEQ) << "bold set";
					break;
				/* faint font (light) - won't support for now, though in theory we easily can. */
				case 2:
					LOG(SEQ_WONT_SUPPORT) << "faint font";
					break;
				/* Italic */
				case 3:
					state_->cell.setFont(state_->cell.font().setItalic());
					LOG(SEQ) << "italics set";
					break;
				/* Underline */
				case 4:
                    state_->cell.setFont(state_->cell.font().setUnderline());
					LOG(SEQ) << "underline set";
					break;
				/* Blinking text */
				case 5:
                    state_->cell.setFont(state_->cell.font().setBlink());
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
                    state_->cell.setFont(state_->cell.font().setStrikethrough());
					LOG(SEQ) << "strikethrough";
					break;
				/* Bold off */
				case 21:
					state_->cell.setFont(state_->cell.font().setBold(false));
					LOG(SEQ) << "bold off";
					break;
				/* Normal - neither bold, nor faint. */
				case 22:
					state_->cell.setFont(state_->cell.font().setBold(false).setItalic(false));
					LOG(SEQ) << "normal font set";
					break;
				/* Italic off. */
				case 23:
					state_->cell.setFont(state_->cell.font().setItalic(false));
					LOG(SEQ) << "italic off";
					break;
				/* Disable underline. */
				case 24:
                    state_->cell.setFont(state_->cell.font().setUnderline(false));
					LOG(SEQ) << "undeline off";
					break;
				/* Disable blinking. */
				case 25:
                    state_->cell.setFont(state_->cell.font().setBlink(false));
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
                    state_->cell.setFont(state_->cell.font().setStrikethrough(false));
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
                break;
            }
            /* OSC 1 - change icon name 
               
               The icon name is a shortened text that should be displayed next to the iconified window. From: https://unix.stackexchange.com/questions/265760/what-does-it-mean-to-set-a-terminals-icon-title
             */
            case 1: {
                // TODO do we want to support this? Not ATM...
                break;
            }
            /* OSC 52 - set clipboard to given value. 
             */
            case 52:
                LOG(SEQ) << "Clipboard set to " << seq.value();
                schedule([this, contents = seq.value()]() {
                    StringEvent::Payload p{contents};
                    onClipboardSetRequest(p, this);
                });
                break;
            /* OSC 112 - reset cursor color. 
             */
            case 112:
                LOG(SEQ) << "Cursor color reset";
                cursor_.setColor(defaultCursor_.color());
                break;
            default:
        		LOG(SEQ_UNKNOWN) << "Invalid OSC sequence: " << seq;
        }
    }



    // ============================================================================================
    // AnsiTerminal::Buffer

    void AnsiTerminal::Buffer::insertLines(int lines, int top, int bottom, Cell const & fill) {
        while (lines-- > 0) {
            Cell * x = rows()[bottom - 1];
            memmove(rows() + top + 1, rows() + top, sizeof(Cell*) * (bottom - top - 1));
            rows()[top] = x;
            fillRow(top, fill, 0, width());
        }
    }

    void AnsiTerminal::Buffer::deleteLines(int lines, int top, int bottom, Cell const & fill, Color defaultBg) {
        while (lines-- > 0) {
            Cell * x = rows()[top];
            // if we are deleting the first line, it gets retired to the history buffer instead. Right-most characters that are empty (i.e. space with no font effects and default background color) are excluded if the line ends with an end of line character
            if (top == 0) {
                int lastCol = width();
                while (lastCol-- > 0) {
                    Cell & c = x[lastCol];
                    // if we have found end of line character, good
                    if (isLineEnd(c))
                        break;
                    // if we have found a visible character, we must remember the whole line, break the search - any end of line characters left of it will make no difference
                    if (c.codepoint() != ' ' || c.bg() != defaultBg || c.font().underline() || c.font().strikethrough()) {
                        break;
                    }
                }
                // if we are not at the end of line, we must remember the whole line
                if (isLineEnd(x[lastCol])) 
                    lastCol += 1;
                else
                    lastCol = width();
                // make the copy and add it to the history line
                // TODO deal with adding history rows, reeenable
                /*
                Cell * row = new Cell[lastCol];
                memcpy(row, x, sizeof(Cell) * lastCol);
                addHistoryRow(lastCol, row);
                */
            }
            memmove(rows() + top, rows() + top + 1, sizeof(Cell*) * (bottom - top - 1));
            rows()[bottom - 1] = x;
            fillRow(bottom - 1, fill, 0, width());
        }
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

    AnsiTerminal::Palette::Palette(std::initializer_list<Color> colors, size_t defaultFg, size_t defaultBg):
        size_(colors.size()),
        defaultFg_(defaultFg),
        defaultBg_(defaultBg),
        colors_(new Color[colors.size()]) {
        ASSERT(defaultFg < size_ && defaultBg < size_);
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
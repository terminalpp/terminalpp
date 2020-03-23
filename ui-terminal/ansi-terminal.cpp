#include "ansi-terminal.h"

namespace ui2 {

    namespace {

        std::unordered_map<Key, std::string> InitializeVT100KeyMap() {
            #define KEY(K, ...) ASSERT(km.find(K) == km.end()) << "Key " << K << " altrady defined"; km.insert(std::make_pair(K, STR(__VA_ARGS__)))
            #define VT_MODIFIERS(K, SEQ1, SEQ2) KEY(Key(K) + Key::Shift, SEQ1 << 2 << SEQ2); \
                                                KEY(Key(K) + Key::Alt, SEQ1 << 3 << SEQ2); \
                                                KEY(Key(K) + Key::Shift + Key::Alt, SEQ1 << 4 << SEQ2); \
                                                KEY(Key(K) + Key::Ctrl, SEQ1 << 5 << SEQ2); \
                                                KEY(Key(K) + Key::Ctrl + Key::Shift, SEQ1 << 6 << SEQ2); \
                                                KEY(Key(K) + Key::Ctrl + Key::Alt, SEQ1 << 7 << SEQ2); \
                                                KEY(Key(K) + Key::Ctrl + Key::Alt + Key::Shift, SEQ1 << 8 << SEQ2);

            using namespace ui2;

            std::unordered_map<Key, std::string> km;
            // first add letter keys in their modifications
            for (unsigned k = 'A'; k <= 'Z'; ++k) {
                // ctrl + letter and ctrl + shift + letter are the same
                KEY(Key(k) + Key::Ctrl, static_cast<char>(k + 1 - 'A'));
                KEY(Key(k) + Key::Ctrl + Key::Shift, static_cast<char>(k + 1 - 'A'));
                // alt simply prepends escape to whatever the non-alt key would be
                KEY(Key(k) + Key::Alt, '\033' << static_cast<char>(k + 32));
                KEY(Key(k) + Key::Shift + Key::Alt, '\033' << static_cast<char>(k));
                KEY(Key(k) + Key::Ctrl + Key::Alt, '\033' << static_cast<char>(k + 1 - 'A'));
                KEY(Key(k) + Key::Ctrl + Key::Shift + Key::Alt, '\033' << static_cast<char>(k + 1 - 'A'));
            }

            // modifiers + numbers
            for (unsigned k = '0'; k <= '9'; ++k) {
                // alt + key prepends escape character
                KEY(Key(k) + Key::Alt, '\033' << static_cast<char>(k));
            }

            // ctrl + 2 is 0
            KEY(Key(Key::Num0) + Key::Ctrl, "\000");
            // alt + shift keys and other extre keys
            KEY(Key(Key::Num0) + Key::Shift + Key::Alt, "\033)");
            KEY(Key(Key::Num1) + Key::Shift + Key::Alt, "\033!");
            KEY(Key(Key::Num2) + Key::Shift + Key::Alt, "\033@");
            KEY(Key(Key::Num3) + Key::Shift + Key::Alt, "\033#");
            KEY(Key(Key::Num4) + Key::Shift + Key::Alt, "\033$");
            KEY(Key(Key::Num5) + Key::Shift + Key::Alt, "\033%");
            KEY(Key(Key::Num6) + Key::Shift + Key::Alt, "\033^");
            KEY(Key(Key::Num7) + Key::Shift + Key::Alt, "\033&");
            KEY(Key(Key::Num8) + Key::Shift + Key::Alt, "\033*");
            KEY(Key(Key::Num9) + Key::Shift + Key::Alt, "\033(");
            // other special characters with alt
            KEY(Key(Key::Tick) + Key::Alt, "\033`");
            KEY(Key(Key::Tick) + Key::Shift + Key::Alt, "\033~");
            KEY(Key(Key::Minus) + Key::Alt, "\033-");
            KEY(Key(Key::Minus) + Key::Alt + Key::Shift, "\033_");
            KEY(Key(Key::Equals) + Key::Alt, "\033=");
            KEY(Key(Key::Equals) + Key::Alt + Key::Shift, "\033+");
            KEY(Key(Key::SquareOpen) + Key::Alt, "\033[");
            KEY(Key(Key::SquareOpen) + Key::Alt + Key::Shift, "\033{");
            KEY(Key(Key::SquareClose) + Key::Alt, "\033]");
            KEY(Key(Key::SquareClose) + Key::Alt + Key::Shift, "\033}");
            KEY(Key(Key::Backslash) + Key::Alt, "\033\\");
            KEY(Key(Key::Backslash) + Key::Alt + Key::Shift, "\033|");
            KEY(Key(Key::Semicolon) + Key::Alt, "\033;");
            KEY(Key(Key::Semicolon) + Key::Alt + Key::Shift, "\033:");
            KEY(Key(Key::Quote) + Key::Alt, "\033'");
            KEY(Key(Key::Quote) + Key::Alt + Key::Shift, "\033\"");
            KEY(Key(Key::Comma) + Key::Alt, "\033,");
            KEY(Key(Key::Comma) + Key::Alt + Key::Shift, "\033<");
            KEY(Key(Key::Dot) + Key::Alt, "\033.");
            KEY(Key(Key::Dot) + Key::Alt + Key::Shift, "\033>");
            KEY(Key(Key::Slash) + Key::Alt, "\033/");
            KEY(Key(Key::Slash) + Key::Alt + Key::Shift, "\033?");
            // arrows, fn keys & friends
            KEY(Key::Up, "\033[A");
            KEY(Key::Down, "\033[B");
            KEY(Key::Right, "\033[C");
            KEY(Key::Left, "\033[D");
            KEY(Key::Home, "\033[H"); // also \033[1~
            KEY(Key::End, "\033[F"); // also \033[4~
            KEY(Key::PageUp, "\033[5~");
            KEY(Key::PageDown, "\033[6~");
            KEY(Key::Insert, "\033[2~");
            KEY(Key::Delete, "\033[3~");
            KEY(Key::F1, "\033OP");
            KEY(Key::F2, "\033OQ");
            KEY(Key::F3, "\033OR");
            KEY(Key::F4, "\033OS");
            KEY(Key::F5, "\033[15~");
            KEY(Key::F6, "\033[17~");
            KEY(Key::F7, "\033[18~");
            KEY(Key::F8, "\033[19~");
            KEY(Key::F9, "\033[20~");
            KEY(Key::F10, "\033[21~");
            KEY(Key::F11, "\033[23~");
            KEY(Key::F12, "\033[24~");

            KEY(Key::Enter, "\r"); // carriage return, not LF
            KEY(Key::Tab, "\t");
            KEY(Key::Esc, "\033");
            KEY(Key::Backspace, "\x7f");

            VT_MODIFIERS(Key::Up, "\033[1;", "A");
            VT_MODIFIERS(Key::Down, "\033[1;", "B");
            VT_MODIFIERS(Key::Left, "\033[1;", "D");
            VT_MODIFIERS(Key::Right, "\033[1;", "C");
            VT_MODIFIERS(Key::Home, "\033[1;", "H");
            VT_MODIFIERS(Key::End, "\033[1;", "F");
            VT_MODIFIERS(Key::PageUp, "\033[5;", "~");
            VT_MODIFIERS(Key::PageDown, "\033[6;", "~");

            VT_MODIFIERS(Key::F1, "\033[1;", "P");
            VT_MODIFIERS(Key::F2, "\033[1;", "Q");
            VT_MODIFIERS(Key::F3, "\033[1;", "R");
            VT_MODIFIERS(Key::F4, "\033[1;", "S");
            VT_MODIFIERS(Key::F5, "\033[15;", "~");
            VT_MODIFIERS(Key::F6, "\033[17;", "~");
            VT_MODIFIERS(Key::F7, "\033[18;", "~");
            VT_MODIFIERS(Key::F8, "\033[19;", "~");
            VT_MODIFIERS(Key::F9, "\033[20;", "~");
            VT_MODIFIERS(Key::F10, "\033[21;", "~");
            VT_MODIFIERS(Key::F11, "\033[23;", "~");
            VT_MODIFIERS(Key::F12, "\033[24;", "~");

            KEY(Key(Key::SquareOpen) + Key::Ctrl, "\033");
            KEY(Key(Key::Backslash) + Key::Ctrl, "\034");
            KEY(Key(Key::SquareClose) + Key::Ctrl, "\035");

            return km;
            #undef KEY
            #undef VT_MODIFIERS
        }

    } // anonymous namespace

    // ============================================================================================

    // TODO remove the underscores
    helpers::Log AnsiTerminal::SEQ("VT100_");
    helpers::Log AnsiTerminal::SEQ_UNKNOWN("VT100_UNKNOWN_");
    helpers::Log AnsiTerminal::SEQ_ERROR("VT100_ERROR_");
    helpers::Log AnsiTerminal::SEQ_WONT_SUPPORT("VT100_WONT_SUPPORT_");

    std::unordered_map<Key, std::string> AnsiTerminal::KeyMap_(InitializeVT100KeyMap());

    char32_t AnsiTerminal::LineDrawingChars_[15] = {0x2518, 0x2510, 0x250c, 0x2514, 0x253c, 0, 0, 0x2500, 0, 0, 0x251c, 0x2524, 0x2534, 0x252c, 0x2502};



    AnsiTerminal::AnsiTerminal(int width, int height, int x, int y):
        Widget{width, height, x, y}, 
        state_{width, height} {

    }

    AnsiTerminal::~AnsiTerminal() {
    }

    void AnsiTerminal::paint(Canvas & canvas) {
        // draw the buffer
        canvas.drawBuffer(state_.buffer, Point{0,0});
    }

    void AnsiTerminal::setRect(Rect const & value) {
        // if resized, resize the terminal buffers and the pty
        // TODO lock the buffer!
        if (value.width() != width() || value.height() != height()) {
            state_.resize(value.width(), value.height());

            ptyResize(value.width(), value.height());
        }
        
        Widget::setRect(value);
    }


    size_t AnsiTerminal::processInput(char const * buffer, char const * bufferEnd) {
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
                    // while this is a code duplication from the helpers::Char class, since this code is a bottleneck for processing large ammounts of text, the code is copied for performance
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
        repaint();
        return bufferEnd - buffer;
    }

    void AnsiTerminal::parseCodepoint(char32_t codepoint) {
        if (state_.lineDrawingSet && codepoint >= 0x6a && codepoint < 0x79)
            codepoint = LineDrawingChars_[codepoint-0x6a];
        LOG(SEQ) << "codepoint " << codepoint << " " << static_cast<char>(codepoint & 0xff);
        updateCursorPosition();
        // set the cell state
        Cell & cell = state_.buffer.at(state_.cursor);
        cell = state_.cell;
        cell.setCodepoint(codepoint);
        // advance cursor's column
        state_.cursor += Point{1, 0};

        // what's left is to deal with corner cases, such as larger fonts & double width characters
        int columnWidth = Char::ColumnWidth(codepoint);

        // if the character's column width is 2 and current font is not double width, update to double width font
        if (columnWidth == 2 && ! cell.font().doubleWidth()) {
            columnWidth = 1;
            cell.setFont(cell.font().setDoubleWidth(true));
        }

        // TODO do double width & height characters properly for the per-line 

        /*
        // if the character's column width is 2 and current font is not double width, update to double width font
        // if the font's size is greater than 1, copy the character as required (if we are at the top row of double height characters, increase the size artificially)
        int charWidth = state_.doubleHeightTopLine ? cell.font().width() * 2 : cell.font().width();

        while (columnWidth > 0 && buffer_.cursor().pos.x < buffer_.cols()) {
            for (int i = 1; (i < charWidth) && buffer_.cursor().pos.x < buffer_.cols(); ++i) {
                Cell& cell2 = buffer_.at(buffer_.cursor().pos);
                // copy current cell properties
                cell2 = cell;
                // make sure the cell's font is normal size and width and display a space
                cell2.setCodepoint(' ').setFont(cell.font().setSize(1).setDoubleWidth(false));
                ++buffer_.cursor().pos.x;
            } 
            if (--columnWidth > 0 && buffer_.cursor().pos.x < buffer_.cols()) {
                Cell& cell2 = buffer_.at(buffer_.cursor().pos);
                // copy current cell properties
                cell2 = cell;
                cell2.setCodepoint(' ');
                ++buffer_.cursor().pos.x;
            } 
        }
        */
    }

    void AnsiTerminal::parseNotification() {
        // NOT_IMPLEMENTED;
    }

    void AnsiTerminal::parseTab() {
        // NOT_IMPLEMENTED;
    }

    void AnsiTerminal::parseLF() {
        // NOT_IMPLEMENTED;
    }

    void AnsiTerminal::parseCR() {
        // NOT_IMPLEMENTED;
    }

    void AnsiTerminal::parseBackspace() {
        // NOT_IMPLEMENTED;
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
            default:
				LOG(SEQ_UNKNOWN) << "Unknown escape sequence \x1b" << *(x-1);
				break;
        }
        return x - buffer;
    }

    void AnsiTerminal::parseCSISequence(CSISequence & seq) {

    }

    void AnsiTerminal::parseOSCSequence(OSCSequence & seq) {

    }


    void AnsiTerminal::updateCursorPosition() {
        while (state_.cursor.x() >= state_.buffer.width()) {
            state_.cursor -= Point{state_.buffer.width(), -1};
            // if the cursor is on the last line, evict the lines above
            if (state_.cursor.y() == state_.scrollEnd) 
                state_.cursor.setY(0);
        }
        // TODO also set the last char position here to the one we update to

    }


    



    // ============================================================================================

    // Palette

    // ============================================================================================

    AnsiTerminal::CSISequence AnsiTerminal::CSISequence::Parse(char const * & start, char const * end) {
        CSISequence result;
        char const * x = start;
        // if we are at the end, return incomplete
        if (x == end) {
            result.firstByte_ = INCOMPLETE;
            return result;
        }
        // parse the first byte
        if (IsParameterByte(*x) && *x != ';' && !helpers::IsDecimalDigit(*x))
            result.firstByte_ = *x++;
        ASSERT(result.firstByte_ != INVALID);
        // parse arguments, if any
        while (x != end && IsParameterByte(*x)) {
            // semicolon separates arguments, in this case an empty argument, which is initialized to default value (0)
            if (*x == ';') {
                ++x;
                result.args_.push_back(std::make_pair(DEFAULT_ARG_VALUE, false));
            // otherwise if we see digit, parse the argument given
            } else if (helpers::IsDecimalDigit(*x)) {
                int arg = 0;
                do {
                    arg = arg * 10 + helpers::DecCharToNumber(*x++);
                } while (x != end && helpers::IsDecimalDigit(*x));
                result.args_.push_back(std::make_pair(arg, true));
                // if there is separator, parse it as well
                if (x != end && *x == ';')
                    ++x;
            // other than numeric values are not supported for now
            } else {
                ++x;
                result.firstByte_ = INVALID;
            }
        }
        // parse intermediate bytes, if there are any, the sequence is marked as invalid because these are not supported now
		while (x != end && IsIntermediateByte(*x)) {
			result.firstByte_ = INVALID;
			++x;
		}
		// parse final byte, first check we are not at the end
		if (x == end) {
            result.firstByte_ = INCOMPLETE;
            return result;
        }
		if (IsFinalByte(*x))
			result.finalByte_ = *x++;
		else
			result.firstByte_ = INVALID;
        // log the sequence if invalid
        if (! result.valid())
			LOG(SEQ_UNKNOWN) << "Unknown, possibly invalid CSI sequence: \x1b" << std::string(start - 1 , x - start + 1);
        start = x;
        return result;
    }

    // ============================================================================================

    AnsiTerminal::OSCSequence AnsiTerminal::OSCSequence::Parse(char const * & start, char const * end) {
        OSCSequence result;
        char const * x = start;
        if (x == end) {
            result.num_ = INCOMPLETE;
            return result;
        }
        // parse the number
        if (helpers::IsDecimalDigit(*x)) {
            int arg = 0;
            do {
                arg = arg * 10 + helpers::DecCharToNumber(*x++);
            } while (x != end && helpers::IsDecimalDigit(*x));
            // if there is no semicolon, keep the INVALID in the number, but continue parsing to BEL or ST
            if (x != end && *x == ';') {
                    ++x;
                    result.num_ = arg;
            } 
        }
        // parse the value, which is terminated by either BEL, or ST, which is ESC followed by backslash
        char const * valueStart = x;
        while (true) {
            if (x == end) {
                result.num_ = INCOMPLETE;
                return result;
            }
            // BEL
            if (*x == helpers::Char::BEL)
                break;
            // ST
            if (*x == helpers::Char::ESC && x + 1 != end && x[1] == '\\') {
                ++x;
                break;
            }
            // next
            ++x;
        }
        result.value_ = std::string(valueStart, x - valueStart);
        ++x; // past the terminating character
        start = x;
        return result;
    }




} // namespace ui
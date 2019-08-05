
#include "helpers/log.h"

#include "vt100.h"



namespace vterm {

    namespace {

        std::unordered_map<ui::Key, std::string> InitializeVT100KeyMap() {
            #define KEY(K, ...) ASSERT(km.find(K) == km.end()) << "Key " << K << " altrady defined"; km.insert(std::make_pair(K, STR(__VA_ARGS__)))
            #define VT_MODIFIERS(K, SEQ1, SEQ2) KEY(Key(K) + Key::Shift, SEQ1 << 2 << SEQ2); \
                                                KEY(Key(K) + Key::Alt, SEQ1 << 3 << SEQ2); \
                                                KEY(Key(K) + Key::Shift + Key::Alt, SEQ1 << 4 << SEQ2); \
                                                KEY(Key(K) + Key::Ctrl, SEQ1 << 5 << SEQ2); \
                                                KEY(Key(K) + Key::Ctrl + Key::Shift, SEQ1 << 6 << SEQ2); \
                                                KEY(Key(K) + Key::Ctrl + Key::Alt, SEQ1 << 7 << SEQ2); \
                                                KEY(Key(K) + Key::Ctrl + Key::Alt + Key::Shift, SEQ1 << 8 << SEQ2);

            using namespace ui;

            std::unordered_map<ui::Key, std::string> km;
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

    // VT100::Palette
    VT100::Palette VT100::Palette::Colors16() {
		return Palette{
			ui::Color::Black(), // 0
			ui::Color::DarkRed(), // 1
			ui::Color::DarkGreen(), // 2
			ui::Color::DarkYellow(), // 3
			ui::Color::DarkBlue(), // 4
			ui::Color::DarkMagenta(), // 5
			ui::Color::DarkCyan(), // 6
			ui::Color::Gray(), // 7
			ui::Color::DarkGray(), // 8
			ui::Color::Red(), // 9
			ui::Color::Green(), // 10
			ui::Color::Yellow(), // 11
			ui::Color::Blue(), // 12
			ui::Color::Magenta(), // 13
			ui::Color::Cyan(), // 14
			ui::Color::White() // 15
		};
    }

    VT100::Palette VT100::Palette::XTerm256() {
        Palette result(256);
        // first the basic 16 colors
		result[0] =	ui::Color::Black();
		result[1] =	ui::Color::DarkRed();
		result[2] =	ui::Color::DarkGreen();
		result[3] =	ui::Color::DarkYellow();
		result[4] =	ui::Color::DarkBlue();
		result[5] =	ui::Color::DarkMagenta();
		result[6] =	ui::Color::DarkCyan();
		result[7] =	ui::Color::Gray();
		result[8] =	ui::Color::DarkGray();
		result[9] =	ui::Color::Red();
		result[10] = ui::Color::Green();
		result[11] = ui::Color::Yellow();
		result[12] = ui::Color::Blue();
		result[13] = ui::Color::Magenta();
		result[14] = ui::Color::Cyan();
		result[15] = ui::Color::White();
		// now do the xterm color cube
		unsigned i = 16;
		for (unsigned r = 0; r < 256; r += 40) {
			for (unsigned g = 0; g < 256; g += 40) {
				for (unsigned b = 0; b < 256; b += 40) {
					result[i] = ui::Color(
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
			result[i] = ui::Color(x, x, x);
			++i;
		}
        return result;
    }

    VT100::Palette::Palette(std::initializer_list<ui::Color> colors, size_t defaultFg, size_t defaultBg):
        size_(colors.size()),
        defaultFg_(defaultFg),
        defaultBg_(defaultBg),
        colors_(new ui::Color[colors.size()]) {
        ASSERT(defaultFg < size_ && defaultBg < size_);
		unsigned i = 0;
		for (ui::Color c : colors)
			colors_[i++] = c;
    }

    VT100::Palette::Palette(Palette const & from):
        size_(from.size_),
        defaultFg_(from.defaultFg_),
        defaultBg_(from.defaultBg_),
        colors_(new ui::Color[from.size_]) {
		memcpy(colors_, from.colors_, sizeof(ui::Color) * size_);
    }

    VT100::Palette::Palette(Palette && from):
        size_(from.size_),
        defaultFg_(from.defaultFg_),
        defaultBg_(from.defaultBg_),
        colors_(from.colors_) {
        from.colors_ = nullptr;
        from.size_ = 0;
    }



    // VT100::CSISequence

    VT100::CSISequence VT100::CSISequence::Parse(char * & start, char const * end) {
        CSISequence result;
        char * x = start;
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
        if (! result.isValid())
			LOG(SEQ_UNKNOWN) << "Unknown, possibly invalid CSI sequence: \x1b" << std::string(start - 1 , x - start + 1);
        start = x;
        return result;
    }

    // VT100

    std::unordered_map<ui::Key, std::string> VT100::KeyMap_(InitializeVT100KeyMap());


    VT100::VT100(int x, int y, int width, int height, Palette const * palette, PTY * pty, size_t ptyBufferSize):
        Terminal{x, y, width, height, pty, ptyBufferSize},
        state_{width, height, ui::Color::White(), ui::Color::Black()},
        mouseMode_{MouseMode::Off},
        mouseEncoding_{MouseEncoding::Default},
        mouseLastButton_{0},
        mouseButtonsDown_{0},
        cursorMode_{CursorMode::Normal},
        palette_(palette) {
    }

    void VT100::updateSize(int cols, int rows) {
        // TODO resize alternate buffer as well
        state_.resize(cols, rows);
        Terminal::updateSize(cols, rows);
    }

    void VT100::mouseDown(int col, int row, ui::MouseButton button, ui::Key modifiers) {
        ASSERT(mouseButtonsDown_ <= 3);
        ++mouseButtonsDown_;
		if (mouseMode_ != MouseMode::Off) {
            mouseLastButton_ = encodeMouseButton(button, modifiers);
            sendMouseEvent(mouseLastButton_, col, row, 'M');
            LOG(SEQ) << "Button " << button << " down at " << col << ";" << row;
        }
        Terminal::mouseUp(col, row, button, modifiers);
    }

    void VT100::mouseUp(int col, int row, ui::MouseButton button, ui::Key modifiers) {
        ASSERT(mouseButtonsDown_ > 0);
        --mouseButtonsDown_;
		if (mouseMode_ == MouseMode::Off) {
            mouseLastButton_ = encodeMouseButton(button, modifiers);
            sendMouseEvent(mouseLastButton_, col, row, 'm');
            LOG(SEQ) << "Button " << button << " up at " << col << ";" << row;
        }
        Terminal::mouseUp(col, row, button, modifiers);
    }

    void VT100::mouseWheel(int col, int row, int by, ui::Key modifiers) {
		if (mouseMode_ != MouseMode::Off) {
    		// mouse wheel adds 64 to the value
            mouseLastButton_ = encodeMouseButton((by > 0) ? ui::MouseButton::Left : ui::MouseButton::Right, modifiers) + 64;
            sendMouseEvent(mouseLastButton_, col, row, 'M');
            LOG(SEQ) << "Wheel offset " << by << " at " << col << ";" << row;
        }
        Terminal::mouseWheel(col, row, by, modifiers);
    }

    void VT100::mouseMove(int col, int row, ui::Key modifiers) {
        if (mouseMode_ != MouseMode::Off && (mouseMode_ != MouseMode::ButtonEvent || mouseButtonsDown_ != 0)) {
            // mouse move adds 32 to the last known button press
            sendMouseEvent(mouseLastButton_ + 32, col, row, 'M');
            LOG(SEQ) << "Mouse moved to " << col << ";" << row;
        }
        Terminal::mouseMove(col, row, modifiers);
    }

    void VT100::keyChar(helpers::Char c) {
		ASSERT(c.codepoint() >= 32);
        send(c.toCharPtr(), c.size());
        Terminal::keyChar(c);
    }

    void VT100::keyDown(ui::Key key) {
		std::string const* seq = GetSequenceForKey(key);
		if (seq != nullptr) {
			switch (key.code()) {
			case ui::Key::Up:
			case ui::Key::Down:
			case ui::Key::Left:
			case ui::Key::Right:
			case ui::Key::Home:
			case ui::Key::End:
				if (key.modifiers() == 0 && cursorMode_ == CursorMode::Application) {
					std::string sa(*seq);
					sa[1] = 'O';
					send(sa.c_str(), sa.size());
					return;
				}
				break;
			default:
				break;
			}
			send(seq->c_str(), seq->size());
		}
        Terminal::keyDown(key);
    }

    void VT100::keyUp(ui::Key key) {
        Terminal::keyUp(key);
    }

    size_t VT100::processInput(char * buffer, size_t bufferSize) {
        {
            Buffer::Ptr guard(this->buffer()); // non-priority lock the buffer
            char * bufferEnd = buffer + bufferSize;
            char * x = buffer;
            while (x != bufferEnd) {
                switch (*x) {
                    /* Parse the escape sequence */
                    case helpers::Char::ESC: {
                        if (! parseEscapeSequence(x, bufferEnd))
                            return x - buffer;
                        break;
                    }
                    case helpers::Char::BEL: {
                        ++x;
                        break;
                    }
                    case helpers::Char::TAB: {
						++x;
						updateCursorPosition();
						if (buffer_.cursor().col % 8 == 0)
							buffer_.cursor().col += 8;
						else
							buffer_.cursor().col += 8 - (buffer_.cursor().col % 8);
						LOG(SEQ) << "Tab: cursor col is " << buffer_.cursor().col;
						break;
                    }
					/* New line simply moves to next line.
					 */
					case helpers::Char::LF: {
						LOG(SEQ) << "LF";
						markLastCharPosition();
						++x;
						// determine if region should be scrolled
						if (++buffer_.cursor().row == state_.scrollEnd) {
							buffer_.deleteLines(1, state_.scrollStart, state_.scrollEnd, (ui::Cell(state_.cell) << ui::Attributes()));
							--buffer_.cursor().row;
						}
						updateCursorPosition();
						setLastCharPosition();
						break;
                    }
					/* Carriage return sets cursor column to 0.
					 */
					case helpers::Char::CR: {
						LOG(SEQ) << "CR";
						markLastCharPosition();
						++x;
						buffer_.cursor().col = 0;
						break;
                    }
					case helpers::Char::BACKSPACE: {
						LOG(SEQ) << "BACKSPACE";
						++x;
						if (buffer_.cursor().col == 0) {
							if (buffer_.cursor().row > 0)
								--buffer_.cursor().row;
							buffer_.cursor().col = buffer_.width() - 1;
						} else {
							--buffer_.cursor().col;
						}
						break;
					}
  					/* default variant is to print the character received to current cell.
					 */
                    default: {
						// make sure that the cursor is in visible part of the screen
						updateCursorPosition();
						// it could be we are dealing with unicode. If entire character was not read, stop the processing, what we have so far will be prepended to next data to be processed
						helpers::Char const * c8 = helpers::Char::At(x, bufferEnd);
						if (c8 == nullptr)
							return x - buffer;
						LOG(SEQ) << "codepoint " << std::hex << c8->codepoint() << " " << static_cast<char>(c8->codepoint() & 0xff);
						// get the cell and update its contents
						Cell& cell = buffer_.at(buffer_.cursor().col, buffer_.cursor().row);
                        cell = state_.cell;
                        cell << c8->codepoint();
						// store the last character position
						setLastCharPosition();
						// move to next column
						++buffer_.cursor().col;
                    }
                }
            }
        }
        repaint();
        return bufferSize;
    }

    bool VT100::parseEscapeSequence(char * & buffer, char const * bufferEnd) {
        ASSERT(*buffer == helpers::Char::ESC);
        char *x = buffer;
        ++x;
        // if we have nothing after the escape character, it is incomplete sequence
        if (x == bufferEnd)
            return false;
        // otherwise, based on the immediate character after ESC, determine what kind of sequence it is
        switch (*x++) {
            /* CSI Sequence */
            case '[': {
                CSISequence seq{CSISequence::Parse(x, bufferEnd)};
                // if the sequence is not valid, it has been reported already and we should just exit
                if (!seq.isValid())
                    break;
                // if the sequence is not complete, return false and do not advance the buffer
                if (!seq.isComplete())
                    return false;
                // otherwise parse the CSI sequence
                parseCSISequence(seq);
                break;
            }
            default:
				LOG(SEQ_UNKNOWN) << "Unknown escape sequence \x1b" << *(x-1);
				break;
        }
        buffer = x;
        return true;
    }

    void VT100::parseCSISequence(CSISequence & seq) {
        switch (seq.firstByte()) {
            // the "normal" CSI sequences
            case 0: 
                switch (seq.finalByte()) {

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

    void VT100::parseCSIGetterOrSetter(CSISequence & seq, bool value) {

    }

    void VT100::parseCSISaveOrRestore(CSISequence & seq) {
		for (size_t i = 0; i < seq.numArgs(); ++i)
			LOG(SEQ_WONT_SUPPORT) << "Private mode " << (seq.finalByte() == 's' ? "save" : "restore") << ", id " << seq[i];
    }

    void VT100::parseSGR(CSISequence & seq) {
        seq.setDefault(0, 0);
		for (size_t i = 0; i < seq.numArgs(); ++i) {
			switch (seq[i]) {
				/* Resets all attributes. */
				case 0:
                    state_.cell << ui::Font() 
                                << ui::Foreground(palette_->defaultForeground())
                                << ui::DecorationColor(palette_->defaultForeground())
                                << ui::Background(palette_->defaultBackground())
                                << ui::Attributes();
					LOG(SEQ) << "font fg bg reset";
					break;
				/* Bold / bright foreground. */
				case 1:
					state_.cell << state_.cell.font().setBold();
					LOG(SEQ) << "bold set";
					break;
				/* faint font (light) - won't support for now, though in theory we easily can. */
				case 2:
					LOG(SEQ_WONT_SUPPORT) << "faint font";
					break;
				/* Italics */
				case 3:
					state_.cell << state_.cell.font().setItalics();
					LOG(SEQ) << "italics set";
					break;
				/* Underline */
				case 4:
                    state_.cell += ui::Attributes::Underline();
					LOG(SEQ) << "underline set";
					break;
				/* Blinking text */
				case 5:
                    state_.cell += ui::Attributes::Blink();
					LOG(SEQ) << "blink set";
					break;
				/* Inverse and inverse off */
				case 7:
				case 27: {
                    ui::Color bg = state_.cell.foreground();
                    ui::Color fg = state_.cell.background();
                    state_.cell << ui::Foreground(fg) << ui::DecorationColor(fg) << ui::Background(bg); 
					LOG(SEQ) << "toggle inverse mode";
					break;
                }
				/* Strikethrough */
				case 9:
                    state_.cell += ui::Attributes::Strikethrough();
					LOG(SEQ) << "strikethrough";
					break;
				/* Bold off */
				case 21:
					state_.cell << state_.cell.font().setBold(false);
					LOG(SEQ) << "bold off";
					break;
				/* Normal - neither bold, nor faint. */
				case 22:
					state_.cell << state_.cell.font().setBold(false).setItalics(false);
					LOG(SEQ) << "normal font set";
					break;
				/* Italics off. */
				case 23:
					state_.cell << state_.cell.font().setItalics(false);
					LOG(SEQ) << "italics off";
					break;
				/* Disable underline. */
				case 24:
                    state_.cell -= ui::Attributes::Underline();
					LOG(SEQ) << "undeline off";
					break;
				/* Disable blinking. */
				case 25:
                    state_.cell -= ui::Attributes::Blink();
					LOG(SEQ) << "blink off";
					break;
				/* Disable strikethrough. */
				case 29:
                    state_.cell -= ui::Attributes::Strikethrough();
					LOG(SEQ) << "Strikethrough off";
					break;
				/* 30 - 37 are dark foreground colors, handled in the default case. */
				/* 38 - extended foreground color */
				case 38: {
                    ui::Color fg = parseSGRExtendedColor(seq, i);
                    state_.cell << ui::Foreground(fg) << ui::DecorationColor(fg);    
					LOG(SEQ) << "fg set to " << fg;
					break;
                }
				/* Foreground default. */
				case 39:
                    state_.cell << ui::Foreground(palette_->defaultForeground())
                                << ui::DecorationColor(palette_->defaultForeground());
					LOG(SEQ) << "fg reset";
					break;
				/* 40 - 47 are dark background color, handled in the default case. */
				/* 48 - extended background color */
				case 48: {
                    ui::Color bg = parseSGRExtendedColor(seq, i);
                    state_.cell << ui::Foreground(bg);    
					LOG(SEQ) << "bg set to " << bg;
					break;
                }
				/* Background default */
				case 49:
					state_.cell << ui::Background(palette_->defaultBackground());
					LOG(SEQ) << "bg reset";
					break;
				/* 90 - 97 are bright foreground colors, handled in the default case. */
				/* 100 - 107 are bright background colors, handled in the default case. */
				default:
					if (seq[i] >= 30 && seq[i] <= 37) {
						state_.cell << ui::Foreground(palette_->at(seq[i] - 30))
                                    << ui::DecorationColor(palette_->at(seq[i] - 30));
						LOG(SEQ) << "fg set to " << palette_->at(seq[i] - 30);
					} else if (seq[i] >= 40 && seq[i] <= 47) {
						state_.cell << ui::Background(palette_->at(seq[i] - 40));
						LOG(SEQ) << "bg set to " << palette_->at(seq[i] - 40);
					} else if (seq[i] >= 90 && seq[i] <= 97) {
						state_.cell << ui::Foreground(palette_->at(seq[i] - 82))
                                    << ui::DecorationColor(palette_->at(seq[i] - 82));
						LOG(SEQ) << "fg set to " << palette_->at(seq[i] - 82);
					} else if (seq[i] >= 100 && seq[i] <= 107) {
						state_.cell << ui::Background(palette_->at(seq[i] - 92));
						LOG(SEQ) << "bg set to " << palette_->at(seq[i] - 92);
					} else {
						LOG(SEQ_UNKNOWN) << "Invalid SGR code: " << seq;
					}
					break;
			}
		}
    }

    ui::Color VT100::parseSGRExtendedColor(CSISequence & seq, size_t & i) {
		++i;
		if (i < seq.numArgs()) {
			switch (seq[i++]) {
				/* index from 256 colors */
				case 5:
					if (i >= seq.numArgs()) // not enough args 
						break;
					if (seq[i] > 255) // invalid color spec
						break;
                    // TODO fix this with palette
                    break;
					//return palette_[seq_[i]];
				/* true color rgb */
				case 2:
					i += 2;
					if (i >= seq.numArgs()) // not enough args
						break;
					if (seq[i - 2] > 255 || seq[i - 1] > 255 || seq[i] > 255) // invalid color spec
						break;
					return ui::Color(seq[i - 2] & 0xff, seq[i - 1] & 0xff, seq[i] & 0xff);
				/* everything else is an error */
				default:
					break;
			}
		}
		LOG(SEQ_UNKNOWN) << "Invalid extended color: " << seq;
		return ui::Color::White();
    }








	unsigned VT100::encodeMouseButton(ui::MouseButton btn, ui::Key modifiers) {
		unsigned result =
			((modifiers | ui::Key::Shift) ? 4 : 0) +
			((modifiers | ui::Key::Alt) ? 8 : 0) +
			((modifiers | ui::Key::Ctrl) ? 16 : 0);
		switch (btn) {
			case ui::MouseButton::Left:
				return result;
			case ui::MouseButton::Right:
				return result + 1;
			case ui::MouseButton::Wheel:
				return result + 2;
			default:
				UNREACHABLE;
		}
	}

	void VT100::sendMouseEvent(unsigned button, int col, int row, char end) {
		// first increment col & row since terminal starts from 1
		++col;
		++row;
		switch (mouseEncoding_) {
			case MouseEncoding::Default: {
				// if the event is release, button number is 3
				if (end == 'm')
					button |= 3;
				// increment all values so that we start at 32
				button += 32;
				col += 32;
				row += 32;
				// if the col & row are too high, ignore the event
				if (col > 255 || row > 255)
					return;
				// otherwise output the sequence
				char buffer[6];
				buffer[0] = '\033';
				buffer[1] = '[';
				buffer[2] = 'M';
				buffer[3] = button & 0xff;
				buffer[4] = col & 0xff;
				buffer[5] = row & 0xff;
				send(buffer, 6);
				break;
			}
			case MouseEncoding::UTF8: {
				LOG(SEQ_WONT_SUPPORT) << "utf8 mouse encoding";
				break;
			}
			case MouseEncoding::SGR: {
				std::string buffer = STR("\033[<" << button << ';' << col << ';' << row << end);
				send(buffer.c_str(), buffer.size());
				break;
			}
		}
	}

    void VT100::updateCursorPosition() {
        int c = buffer_.width();
        while (buffer_.cursor().col >= c) {
            buffer_.cursor().col -= c;
            if (++buffer_.cursor().row == state_.scrollEnd) {
                buffer_.deleteLines(1, state_.scrollStart, state_.scrollEnd, ui::Cell(state_.cell) << ui::Attributes());
                --buffer_.cursor().row;
            }
        }
        ASSERT(buffer_.cursor().col < buffer_.width());
        // if cursor row is not valid, just set it to the last row 
        if (buffer_.cursor().row >= buffer_.height())
            buffer_.cursor().row = buffer_.height() - 1;
    }




} // namespace vterm
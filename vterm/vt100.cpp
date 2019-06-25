#include <algorithm>
#include <iostream>


#include "helpers/strings.h"
#include "helpers/log.h"
#include "helpers/base64.h"

#include "vt100.h"



namespace vterm {

	VT100::KeyMap VT100::keyMap_;

    // Palette

	Palette Palette::Colors16() {
        return Palette{
            Color::Black(), // 0
            Color::DarkRed(), // 1
            Color::DarkGreen(), // 2
            Color::DarkYellow(), // 3
            Color::DarkBlue(), // 4
            Color::DarkMagenta(), // 5
            Color::DarkCyan(), // 6
            Color::Gray(), // 7
            Color::DarkGray(), // 8
            Color::Red(), // 9
            Color::Green(), // 10
            Color::Yellow(), // 11
            Color::Blue(), // 12
            Color::Magenta(), // 13
            Color::Cyan(), // 14
            Color::White() // 15
        };
	}

    Palette Palette::ColorsXTerm256() {
        Palette result(256);
        // start with the 16 colors
        result.fillFrom(Colors16());
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
        for (unsigned char x = 8; x <= 238; x +=10) {
            result[i] = Color(x, x, x);
            ++i;
        }
        return result;
    }


	Palette::Palette(std::initializer_list<Color> colors):
		size_(colors.size()),
		colors_(new Color[colors.size()]) {
		unsigned i = 0;
		for (Color c : colors)
			colors_[i++] = c;
	}

	Palette::Palette(Palette const & from) :
		size_(from.size_),
		colors_(new Color[from.size_]) {
		memcpy(colors_, from.colors_, sizeof(Color) * size_);
	}

	Palette::Palette(Palette && from) :
		size_(from.size_),
		colors_(from.colors_) {
		from.size_ = 0;
		from.colors_ = nullptr;
	}

	void Palette::fillFrom(Palette const & from) {
		size_t s = std::min(size_, from.size_);
		std::memcpy(colors_, from.colors_, sizeof(Color) * s);
	}

    // VT100::KeyMap

	/** showkey -a shows what keys were pressed in a linux environment. 
	 */
	VT100::KeyMap::KeyMap() {
		// shift + ctrl + alt + meta modifiers and their combinations
		keys_.resize(16);
		// add modifiers + letter
		for (unsigned k = 'A'; k <= 'Z'; ++k) {
			// ctrl + letter and ctrl + shift + letter are the same
			addKey(Key(k) + Key::Ctrl, STR(static_cast<char>(k + 1 - 'A')).c_str());
			addKey(Key(k) + Key::Ctrl + Key::Shift, STR(static_cast<char>(k + 1 - 'A')).c_str());
			// alt simply prepends escape to whatever the non-alt key would be
			addKey(Key(k) + Key::Alt, STR('\033' << static_cast<char>(k + 32)).c_str());
			addKey(Key(k) + Key::Shift + Key::Alt, STR('\033' << static_cast<char>(k)).c_str());
			addKey(Key(k) + Key::Ctrl + Key::Alt, STR('\033' << static_cast<char>(k + 1 - 'A')).c_str());
			addKey(Key(k) + Key::Ctrl + Key::Shift + Key::Alt, STR('\033' << static_cast<char>(k + 1 - 'A')).c_str());
		}
		// modifiers + numbers
		for (unsigned k = '0'; k <= '9'; ++k) {
			// alt + key prepends escape character
			addKey(Key(k) + Key::Alt, STR('\033' << static_cast<char>(k)).c_str());
		}
		// ctrl + 2 is 0
		addKey(Key(Key::Num0) + Key::Ctrl, "\000");
		// alt + shift keys and other extre keys
		addKey(Key(Key::Num0) + Key::Shift + Key::Alt, "\033)");
		addKey(Key(Key::Num1) + Key::Shift + Key::Alt, "\033!");
		addKey(Key(Key::Num2) + Key::Shift + Key::Alt, "\033@");
		addKey(Key(Key::Num3) + Key::Shift + Key::Alt, "\033#");
		addKey(Key(Key::Num4) + Key::Shift + Key::Alt, "\033$");
		addKey(Key(Key::Num5) + Key::Shift + Key::Alt, "\033%");
		addKey(Key(Key::Num6) + Key::Shift + Key::Alt, "\033^");
		addKey(Key(Key::Num7) + Key::Shift + Key::Alt, "\033&");
		addKey(Key(Key::Num8) + Key::Shift + Key::Alt, "\033*");
		addKey(Key(Key::Num9) + Key::Shift + Key::Alt, "\033(");
		// other special characters with alt
		addKey(Key(Key::Tick) + Key::Alt, "\033`");
		addKey(Key(Key::Tick) + Key::Shift + Key::Alt, "\033~");
		addKey(Key(Key::Minus) + Key::Alt, "\033-");
		addKey(Key(Key::Minus) + Key::Alt + Key::Shift, "\033_");
		addKey(Key(Key::Equals) + Key::Alt, "\033=");
		addKey(Key(Key::Equals) + Key::Alt + Key::Shift, "\033+");
		addKey(Key(Key::SquareOpen) + Key::Alt, "\033[");
		addKey(Key(Key::SquareOpen) + Key::Alt + Key::Shift, "\033{");
		addKey(Key(Key::SquareClose) + Key::Alt, "\033]");
		addKey(Key(Key::SquareClose) + Key::Alt + Key::Shift, "\033}");
		addKey(Key(Key::Backslash) + Key::Alt, "\033\\");
		addKey(Key(Key::Backslash) + Key::Alt + Key::Shift, "\033|");
		addKey(Key(Key::Semicolon) + Key::Alt, "\033;");
		addKey(Key(Key::Semicolon) + Key::Alt + Key::Shift, "\033:");
		addKey(Key(Key::Quote) + Key::Alt, "\033'");
		addKey(Key(Key::Quote) + Key::Alt + Key::Shift, "\033\"");
		addKey(Key(Key::Comma) + Key::Alt, "\033,");
		addKey(Key(Key::Comma) + Key::Alt + Key::Shift, "\033<");
		addKey(Key(Key::Dot) + Key::Alt, "\033.");
		addKey(Key(Key::Dot) + Key::Alt + Key::Shift, "\033>");
		addKey(Key(Key::Slash) + Key::Alt, "\033/");
		addKey(Key(Key::Slash) + Key::Alt + Key::Shift, "\033?");
		// arrows, fn keys & friends
		addKey(Key::Up, "\033[A");
		addKey(Key::Down, "\033[B");
		addKey(Key::Right, "\033[C");
		addKey(Key::Left, "\033[D");
		addKey(Key::Home, "\033[H"); // also \033[1~
		addKey(Key::End, "\033[F"); // also \033[4~
		addKey(Key::PageUp, "\033[5~");
		addKey(Key::PageDown, "\033[6~");
		addKey(Key::Insert, "\033[2~");
		addKey(Key::Delete, "\033[3~");
		addKey(Key::F1, "\033OP");
		addKey(Key::F2, "\033OQ");
		addKey(Key::F3, "\033OR");
		addKey(Key::F4, "\033OS");
		addKey(Key::F5, "\033[15~");
		addKey(Key::F6, "\033[17~");
		addKey(Key::F7, "\033[18~");
		addKey(Key::F8, "\033[19~");
		addKey(Key::F9, "\033[20~");
		addKey(Key::F10, "\033[21~");
		addKey(Key::F11, "\033[23~");
		addKey(Key::F12, "\033[24~");

		addKey(Key::Enter, "\r"); // carriage return, not LF
		addKey(Key::Tab, "\t");
		addKey(Key::Esc, "\033");
		addKey(Key::Backspace, "\x7f");

		addVTModifiers(Key::Up, "\033[1;", "A");
		addVTModifiers(Key::Down, "\033[1;", "B");
		addVTModifiers(Key::Left, "\033[1;", "D");
		addVTModifiers(Key::Right, "\033[1;", "C");
		addVTModifiers(Key::Home, "\033[1;", "H");
		addVTModifiers(Key::End, "\033[1;", "F");

		addVTModifiers(Key::F1, "\033[1;", "P");
		addVTModifiers(Key::F2, "\033[1;", "Q");
		addVTModifiers(Key::F3, "\033[1;", "R");
		addVTModifiers(Key::F4, "\033[1;", "S");
		addVTModifiers(Key::F5, "\033[15;", "~");
		addVTModifiers(Key::F6, "\033[17;", "~");
		addVTModifiers(Key::F7, "\033[18;", "~");
		addVTModifiers(Key::F8, "\033[19;", "~");
		addVTModifiers(Key::F9, "\033[20;", "~");
		addVTModifiers(Key::F10, "\033[21;", "~");
		addVTModifiers(Key::F11, "\033[23;", "~");
		addVTModifiers(Key::F12, "\033[24;", "~");

		addKey(Key(Key::SquareOpen) + Key::Ctrl, "\033");
		addKey(Key(Key::Backslash) + Key::Ctrl, "\034");
		addKey(Key(Key::SquareClose) + Key::Ctrl, "\035");
	}

	std::string const* VT100::KeyMap::getSequence(Key k) {
		auto & m = getModifierMap(k);
		auto i = m.find(k.code());
		if (i == m.end())
			return nullptr;
		else
			return & (i->second);
	}

	void VT100::KeyMap::addKey(Key k, char const * seq) {
		auto& m = getModifierMap(k);
		ASSERT(m.find(k.code()) == m.end());
		m[k.code()] = std::string(seq);
	}

	void VT100::KeyMap::addVTModifiers(Key k, char const* seq1, char const* seq2) {
		std::string shift = STR(seq1 << 2 << seq2); 
		std::string alt = STR(seq1 << 3 << seq2); 
		std::string alt_shift = STR(seq1 << 4 << seq2); 
		std::string ctrl = STR(seq1 << 5 << seq2); 
		std::string ctrl_shift = STR(seq1 << 6 << seq2); 
		std::string ctrl_alt = STR(seq1 << 7 << seq2); 
		std::string ctrl_alt_shift = STR(seq1 << 8 << seq2); 
		addKey(k + Key::Shift, shift.c_str());
		addKey(k + Key::Alt, alt.c_str());
		addKey(k + Key::Shift + Key::Alt, alt_shift.c_str());
		addKey(k + Key::Ctrl, ctrl.c_str());
		addKey(k + Key::Ctrl + Key::Shift, ctrl_shift.c_str());
		addKey(k + Key::Ctrl + Key::Alt, ctrl_alt.c_str());
		addKey(k + Key::Ctrl + Key::Alt + Key::Shift, ctrl_alt_shift.c_str());
	}

	std::unordered_map<unsigned, std::string>& VT100::KeyMap::getModifierMap(Key k) {
		unsigned m = (k.modifiers() >> 16);
		ASSERT(m < 16);
		return keys_[m];
	}

    // VT100::InvalidCSISequence

    VT100::InvalidCSISequence::InvalidCSISequence(VT100::CSISequence const & seq) {
        what_ = STR("Invalid CSI sequence: " << seq);
    }
    VT100::InvalidCSISequence::InvalidCSISequence(std::string const & msg, VT100::CSISequence const & seq) {
        what_ = STR(msg << seq);
    }

    // VT100::CSISequence

    bool VT100::CSISequence::parse(VT100 & term) {
        bool throwAtEnd = false;
        start_ = term.buffer_;
        char c = term.top();
        // if the first char is parameter byte other than number or semicolon, store it in the firstByte
        if (IsParameterByte(c) && (c != ';') && ! helpers::IsDecimalDigit(c) ) {
            firstByte_ = term.pop();
            c = term.top();
        }
        // now we parse arguments
        while (IsParameterByte(c)) {
            // skip argument
            if (c == ';') {
                term.pop();
                args_.push_back(std::make_pair(0, false));
            } else if (helpers::IsDecimalDigit(c)) {
                args_.push_back(std::make_pair(term.parseNumber(), true));
                if (term.condPop(';')) {
                    c = term.top();
                    continue;
                }
            } else {
                term.pop();
                throwAtEnd = true; // unsupported parameter character
            }
            c = term.top();
        }
        // now check the intermediate bytes
        while (IsIntermediateByte(term.top())) {
            term.pop();
            throwAtEnd = true;
        }
		// if there is no final byte, that means we need more data, therefore return false
		if (term.eof())
			return false;
		// parse the final byte, or if the next character is not valid, mark to throw an error
		if (IsFinalByte(term.top()))
			finalByte_ = term.pop();
		else
			throwAtEnd = true;
        end_ = term.buffer_;
        // if unsupported structure was encountered, throw the error
        if (throwAtEnd)
            THROW(InvalidCSISequence("Unsupported sequence structure :", *this));
        return true;
    } 

    // VT100

	VT100::VT100(PTY * pty, Palette const & palette, unsigned defaultFg, unsigned defaultBg) :
		PTYBackend(pty),
		palette_(256),
		defaultFg_(defaultFg),
		defaultBg_(defaultBg),
        state_{palette[defaultFg], palette[defaultBg], Font(), 0, 0},
        otherState_{palette[defaultFg], palette[defaultBg], Font(), 0, 0},
        otherBuffer_(0,0),
        applicationKeypadMode_(false),
        applicationCursorMode_(false),
		alternateBuffer_(false),
        bracketedPaste_(false),
		invalidateAll_(false),
		mouseMode_(MouseMode::Off),
		mouseEncoding_(MouseEncoding::Default),
		mouseLastButton_(0),
	    buffer_(nullptr),
	    bufferEnd_(nullptr) {
		palette_.fillFrom(palette);
	}

	void VT100::keyDown(Key k) {
		if (k | Key::Shift)
			inputState_.shift = true;
		if (k | Key::Ctrl)
			inputState_.ctrl = true;
		if (k | Key::Alt)
			inputState_.alt = true;
		if (k | Key::Win)
			inputState_.win = true;
		std::string const* seq = keyMap_.getSequence(k);
		if (seq != nullptr) {
            switch (k.code()) {
                case Key::Up:
                case Key::Down:
                case Key::Left:
                case Key::Right:
                case Key::Home:
                case Key::End:
                    if (k.modifiers() == 0 && applicationCursorMode_) {
                        std::string sa(*seq);
                        sa[1] = 'O';
                        sendData(sa.c_str(), sa.size());
                        return;
                    }
                    break;
                default:
                    break;
            }
			sendData(seq->c_str(), seq->size());
        }
		//LOG << "Key pressed: " << k;
	}

	void VT100::keyUp(Key k) {
		if (k | Key::Shift)
			inputState_.shift = false;
		if (k | Key::Ctrl)
			inputState_.ctrl = false;
		if (k | Key::Alt)
			inputState_.alt = false;
		if (k | Key::Win)
			inputState_.win = false;

	}

	void VT100::keyChar(Char::UTF8 c) {
		// TODO make sure that the character is within acceptable range, i.e. it does not clash with ctrl+ stuff, etc. 
		sendData(c.rawBytes(), c.size());
	}

	void VT100::mouseMove(unsigned col, unsigned row) {
		if (mouseMode_ == MouseMode::Off)
			return;
		if (mouseMode_ == MouseMode::ButtonEvent && !inputState_.mouseLeft && !inputState_.mouseRight && !inputState_.mouseWheel)
			return;
		// mouse move adds 32 to the last known button press
		sendMouseEvent(mouseLastButton_ + 32, col, row, 'M');
		LOG(SEQ) << "Mouse moved to " << col << ";" << row;
	}

	void VT100::mouseDown(unsigned col, unsigned row, MouseButton button) {
		switch (button) {
			case MouseButton::Left:
				inputState_.mouseLeft = true;
				break;
			case MouseButton::Right:
				inputState_.mouseRight = true;
				break;
			case MouseButton::Wheel:
				inputState_.mouseWheel = true;
				break;
		}
		if (mouseMode_ == MouseMode::Off)
			return;
		mouseLastButton_ = encodeMouseButton(button);
		sendMouseEvent(mouseLastButton_, col, row, 'M');
		LOG(SEQ) << "Button " << button << " down at " << col << ";" << row;
	}

	void VT100::mouseUp(unsigned col, unsigned row, MouseButton button) {
		switch (button) {
			case MouseButton::Left:
				inputState_.mouseLeft = false;
				break;
			case MouseButton::Right:
				inputState_.mouseRight = false;
				break;
			case MouseButton::Wheel:
				inputState_.mouseWheel = false;
				break;
			}
		if (mouseMode_ == MouseMode::Off)
			return;
		mouseLastButton_ = encodeMouseButton(button);
		sendMouseEvent(mouseLastButton_, col, row, 'm');
		LOG(SEQ) << "Button " << button << " up at " << col << ";" << row;
	}

	void VT100::mouseWheel(unsigned col, unsigned row, int offset) {
		if (mouseMode_ == MouseMode::Off)
			return;
		// mouse wheel adds 64 to the value
		mouseLastButton_ = encodeMouseButton((offset > 0) ? MouseButton::Left : MouseButton::Right) + 64;
		sendMouseEvent(mouseLastButton_, col, row, 'M');
		LOG(SEQ) << "Wheel offset " << offset << " at " << col << ";" << row;
	}

    void VT100::paste(std::string const & what) {
        if (bracketedPaste_) {
            sendData("\033[200~",6);
            sendData(what.c_str(), what.size());
            sendData("\033[201~",6);
        } else {
            sendData(what.c_str(), what.size());
        }
    }

	void VT100::resize(unsigned cols, unsigned rows) {
        // reset the scroll region to whole window
        state_.resize(cols, rows);
        otherState_.resize(cols, rows);
		// and resize its cells
        resizeBuffer(otherBuffer_,cols, rows);
		// call the PTY backend's resize which propagates the change to the pty
		PTYBackend::resize(cols, rows);
		LOG(SEQ) << "terminal resized to " << cols << "," << rows;
	}

	size_t VT100::dataReceived(char * buffer, size_t size) {
		invalidateAll_ = false;
		buffer_ = buffer;
		bufferEnd_ = buffer_ + size;
		std::string text;
		while (! eof()) {
			char c = top();
			switch (c) {
			case Char::ESC: {
				if (!text.empty()) {
					LOG(SEQ) << "text " << text;
					text.clear();
				}
				// make a copy of buffer in case process escape sequence advances it and then fails
				char * b = buffer_;
                if (! parseEscapeSequence())
                    return b - buffer;
				break;
			}
            /* BEL is sent if user should be notified, i.e. play a notification sound or so. 
                */
            case Char::BEL:
                pop();
				LOG(SEQ) << "BEL notification";
				notify();
                break;
            case Char::TAB:
                // TODO tabstops and stuff
                pop();
                updateCursorPosition();
                if (cursor().col % 8 == 0) 
                    cursor().col += 8;
                else
                    cursor().col += 8 - (cursor().col % 8);
                LOG(SEQ) << "Tab: cursor col is " << cursor().col;
                break;
			/* New line simply moves to next line.
				*/
			case Char::LF:
				LOG(SEQ) << "LF";
				pop();
				++cursor().row;
				updateCursorPosition();
				break;
			/* Carriage return sets cursor column to 0. 
				*/
			case Char::CR:
				LOG(SEQ) << "CR";
				pop();
				cursor().col = 0;
				break;
			case Char::BACKSPACE: {
				if (cursor().col == 0) {
					if (cursor().row > 0)
						--cursor().row;
					cursor().col = cols() - 1;
				} else {
					--cursor().col;
				}
				// TODO backspace behaves weirdly - it seems we do not need to actually delete the character at all, in fact the whole backspace character seems to make very little difference
		  		//Cell& cell = defaultLayer_->at(cursor().col, cursor().row);
				//cell.c = ' ';
				pop();
				break;
			}
			/* default variant is to print the character received to current cell.
				*/
			default: {
				// make sure that the cursor is in visible part of the screen
				updateCursorPosition();
				// it could be we are dealing with unicode
				Char::UTF8 c8;
				if (! c8.readFromStream(buffer_, bufferEnd_)) 
					return buffer_ - buffer;
				// TODO fix this to utf8
				text += static_cast<char>(c8.codepoint() & 0xff);
				//LOG(SEQ) << "codepoint " << std::hex << c8.codepoint() << " " << static_cast<char>(c8.codepoint() & 0xff);
				// get the cell and update its contents
				Terminal::Cell & cell = this->buffer().at(cursor().col, cursor().row);
				cell.fg = state_.fg;
				cell.bg = state_.bg;
				cell.font = state_.font;
				cell.c = c8;
				cell.dirty = true;
				// move to next column
				setCursor(cursor().col + 1, cursor().row);
			}
			}
		}
		if (!text.empty()) {
			LOG(SEQ) << "text " << text;
			text.clear();
		}
		terminal()->repaint(invalidateAll_);
		return size;
	}

	bool VT100::skipEscapeSequence() {
		ASSERT(*buffer_ == Char::ESC);
		pop();
		if (eof())
			return false;
		// if the next character is `[` then we match CSI, otherwise we just sklip it as the second byte of the sequence
		switch (pop()) {
			case '[': {
				// the CSI is now followed by arbitrary number parameter bytes (0x30 - 0x3f)
				while (top() >= 0x30 && top() <= 0x3f)
					pop();
				// then by any number of intermediate bytes (0x20 - 0x2f)
				while (top() >= 0x20 && top() <= 0x2f)
					pop();
				// and then by the final byte
				if (eof())
					return false;
				pop();
				break;
			}
			case ']': {
				// TODO for now we only handle BEL
				while (! condPop(Char::BEL)) {
					if (eof())
						return false;
					pop();
				}
				break;
			}
		}
		return true;
	}

	unsigned VT100::parseNumber(unsigned defaultValue) {
		if (!helpers::IsDecimalDigit(top()))
			return defaultValue;
		int value = 0;
		while (helpers::IsDecimalDigit(top()))
			value = value * 10 + helpers::DecCharToNumber(pop());
		return value;
	}

    /** Returns true if valid escape sequence has been parsed from the buffer regardless of whether that sequence is supported or not. Return false if an incomplete possibly valid sequence has been found and therefore more data must be read from the terminal in order to proceed. 
     */
	bool VT100::parseEscapeSequence() {
		ASSERT(*buffer_ == Char::ESC);
		pop();
        // if at eof now, we need to read more so that we know what to escape
        if (eof())
            return false;
        char c = pop();
		switch (c) {
            /* Reverse line feed - move up 1 row, same column.
             */
            case 'M':
                LOG(SEQ) << "RI: move cursor 1 line up";
                setCursor(cursor().row - 1, cursor().col);
                return true;
            /* Operating system command. */
		    case ']':
				return parseOSC();
            /* CSI Sequence - parse using the CSISequence class. */
			case '[': {
                try {
                    CSISequence csi;
                    if (! csi.parse(*this))
                        return false;
                    // TODO now actually do what the sequence says
                    parseCSI(csi);
                    return true;
                } catch (InvalidCSISequence const & e) {
                    LOG(SEQ_UNKNOWN) << e;
                    return true;
	            }
            }
            /* Character set specification - ignored, we just have to parse it. */
            case '(':
            case ')':
            case '*':
            case '+':
                // missing character set specification
                if (eof())
                    return false;
                if (condPop('B')) // US
                    return true;
                LOG(SEQ_WONT_SUPPORT) << "Unknown (possibly mismatched) character set final char " << pop();
                return true;
            /* ESC = -- Application keypad */
            case '=':
                LOG(SEQ) << "Application keypad mode enabled";
                applicationKeypadMode_ = true;
                return true;
            /* ESC > -- Normal keypad */
            case '>':
                LOG(SEQ) << "Normal keypad mode enabled";
                applicationKeypadMode_ = false;
                return true;
            /* Otherwise we have unknown escape sequence. This is an issue since we do not know when it ends and therefore may break the parsing.
             */
			default:
                LOG(SEQ_UNKNOWN) << "Unknown (possibly mismatched) char after ESC " << c;
				return true;
		}
	}

	void VT100::parseCSI(CSISequence & seq) {
        if (seq.firstByte() == '?') {
            switch (seq.finalByte()) {
                /* Multiple options can be set high or low in a single command. 
                 */
                case 'h':
                case 'l':
                    return parseSetterOrGetter(seq);
				case 's':
				case 'r':
					return parseSaveOrRestore(seq);
                default:
                    break;
            }
        } else if (seq.firstByte() == '>') {
            switch (seq.finalByte()) {
                /* CSI > 0 c -- Send secondary device attributes. 
                 */
                case 'c':
                    if (seq[0] != 0)
                        break;
                    LOG(SEQ) << "Secondary Device Attributes - VT100 sent";
                    sendData("\033[>0;0;0c",5); // we are VT100, no version third must always be zero (ROM cartridge)
                    return;
                default:
                    break;
            }
        } else if (seq.firstByte() == 0) {
            switch (seq.finalByte()) {
                // CSI <n> A -- moves cursor n rows up (CUU)
				case 'A': {
					seq.setArgDefault(0, 1);
					ASSERT(seq.numArgs() == 1);
					unsigned r = cursor().row >= seq[0] ? cursor().row - seq[0] : 0;
					LOG(SEQ) << "CUU: setCursor " << cursor().col << ", " << r;
					setCursor(cursor().col, r);
					return;
				}
                // CSI <n> B -- moves cursor n rows down (CUD)
                case 'B':
                    seq.setArgDefault(0, 1);
                    ASSERT(seq.numArgs() == 1);
                    LOG(SEQ) << "CUD: setCursor " << cursor().col << ", " << cursor().row + seq[0];
                    setCursor(cursor().col, cursor().row + seq[0]);
                    return;
                // CSI <n> C -- moves cursor n columns forward (right) (CUF)
                case 'C':
                    seq.setArgDefault(0, 1);
                    ASSERT(seq.numArgs() == 1);
                    LOG(SEQ) << "CUF: setCursor " << cursor().col + seq[0] << ", " << cursor().row;
                    setCursor(cursor().col + seq[0], cursor().row);
                    return;
                // CSI <n> D -- moves cursor n columns back (left) (CUB)
				case 'D': {// cursor backward
					seq.setArgDefault(0, 1);
					ASSERT(seq.numArgs() == 1);
					unsigned c = cursor().col >= seq[0] ? cursor().col - seq[0] : 0;
					LOG(SEQ) << "CUB: setCursor " << c << ", " << cursor().row;
					setCursor(c, cursor().row);
					return;
				}
                /* CSI <n> G -- set cursor character absolute
                 */
                case 'G': // CHA
                    seq.setArgDefault(0,1);
                    LOG(SEQ) << "CHA: set column " << seq[0] - 1;
                    setCursor(seq[0] - 1, cursor().row);
                    return;
                /* set cursor position (CUP) */
                case 'H': // CUP
                case 'f': // HVP
                    seq.setArgDefault(0, 1);
                    seq.setArgDefault(1, 1);
                    ASSERT(seq.numArgs() == 2);
                    LOG(SEQ) << "CUP: setCursor " << seq[1] - 1 << ", " << seq[0] - 1;
                    setCursor(seq[1] - 1, seq[0] - 1);
                    return;
                /* CSI <n> J -- erase display, depending on <n>:
                    0 = erase from the current position (inclusive) to the end of display
                    1 = erase from the beginning to the current position(inclusive)
                    2 = erase entire display
                */
                case 'J':
                    ASSERT(seq.numArgs() <= 1);
                    switch (seq[0]) {
                        case 0:
                            updateCursorPosition();
                            fillRect(helpers::Rect(cursor().col, cursor().row, cols(), cursor().row + 1), ' ');
                            fillRect(helpers::Rect(0, cursor().row + 1, cols(), rows()), ' ');
                            return;
                        case 1:
                            updateCursorPosition();
                            fillRect(helpers::Rect(0, 0, cols(), cursor().row), ' ');
                            fillRect(helpers::Rect(0, cursor().row, cursor().col + 1, cursor().row + 1), ' ');
                            return;
                        case 2:
                            fillRect(helpers::Rect(cols(), rows()), ' ');
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
                    ASSERT(seq.numArgs() <= 1);
                    switch (seq[0]) {
                        case 0:
                            updateCursorPosition();
                            fillRect(helpers::Rect(cursor().col, cursor().row, cols(), cursor().row + 1), ' ');
                            return;
                        case 1:
                            updateCursorPosition();
                            fillRect(helpers::Rect(0, cursor().row, cursor().col + 1, cursor().row + 1), ' '); 
                            return;
                        case 2:
                            updateCursorPosition();
                            fillRect(helpers::Rect(0, cursor().row, cols(), cursor().row + 1), ' ');
                            return;
                        default:
                            break;
                    }
                    break;
                /* CSI <n> L -- Insert n lines. (IL)
                 */
                case 'L':
                    seq.setArgDefault(0, 1);
                    LOG(SEQ) << "IL: scrollUp " << seq[0]; 
                    insertLine(seq[0], cursor().row);
                    return;
                /* CSI <n> M -- Remove n lines. (DL)
                 */
                case 'M':
                    seq.setArgDefault(0,1);
                    LOG(SEQ) << "DL: scrollDown " << seq[0]; 
                    deleteLine(seq[0], cursor().row);
                    return;
				/* CSI <n> P -- Delete n charcters. (DCH) */
				case 'P':
					seq.setArgDefault(0, 1);
					LOG(SEQ) << "DCH: deleteCharacter " << seq[0];
					deleteCharacters(seq[0]);
					return;
				/* CSI <n> S -- Scroll up n lines 
				 */
				case 'S':
					seq.setArgDefault(0, 1);
					LOG(SEQ) << "SU: scrollUp " << seq[0];
					deleteLine(seq[0], state_.scrollStart);
					return;
					/* CSI <n> T -- Scroll down n lines
                 */
                case 'T':
                    seq.setArgDefault(0, 1);
                    LOG(SEQ) << "SD: scrollDown " << seq[0];
					// TODO should this be from cursor, or from scrollStart? 
                    insertLine(seq[0], cursor().row);
                    return;
                /* CSI <n> X -- erase <n> characters from the current position 
                */
                case 'X': {
                    seq.setArgDefault(0, 1);
                    ASSERT(seq.numArgs() == 1);
                    updateCursorPosition();
                    // erase from first line
                    unsigned n = static_cast<unsigned>(seq[0]);
                    unsigned l = std::min(cols() - cursor().col, n);
                    fillRect(helpers::Rect(cursor().col, cursor().row, cursor().col + l, cursor().row + 1), ' ');
                    n -= l;
                    // while there is enough stuff left to be larger than a line, erase entire line
                    l = cursor().row + 1;
                    while (n >= cols() && l < rows()) {
                        fillRect(helpers::Rect(0, l, cols(), l + 1), ' ');
                        ++l;
                        n -= cols();
                    }
                    // if there is still something to erase, erase from the beginning
                    if (n != 0 && l < rows()) 
                        fillRect(helpers::Rect(0, l, n, l + 1), ' ');
                    return;
                }
                /* CSI <n> c - primary device attributes.
                 */
                case 'c': {
                    if (seq[0] != 0)
                        break;
                    LOG(SEQ) << "Device Attributes - VT102 sent";
                    sendData("\033[?6c",5); // send VT-102 for now, go for VT-220? 
                    return;
                }
                /* CSI <n> d -- Line position absolute (VPA)
                 */
                case 'd': {
                    seq.setArgDefault(0, 1);
                    if (seq.numArgs() != 1)
                        break;
                    unsigned r = seq[0];
                    if (r < 1)
                        r = 1;
                    else if (r > rows())
                        r = rows();
                    LOG(SEQ) << "VPA: setCursor " << cursor().col << ", " << r - 1;
                    setCursor(cursor().col, r - 1);
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
                    seq.setArgDefault(0,0);
                    // enable replace mode (IRM) since this is the only mode we allow, do nothing
                    if (seq[0] == 4) 
                        return;
                    break;
                /* SGR 
                */
                case 'm':
                    return parseSGR(seq);
                /* CSI <n> ; <n> r -- Set scrolling region (default is the whole window)
                 */
                case 'r':
                    seq.setArgDefault(0, 1); // inclusive
                    seq.setArgDefault(1, rows()); // inclusive
                    if (seq.numArgs() != 2)
                        break;
                    state_.scrollStart = std::min(seq[0] - 1, rows() - 1); // inclusive
                    state_.scrollEnd = std::min(seq[1] , rows()); // exclusive 
					LOG(SEQ) << "Scroll region set to " << state_.scrollStart << " - " << state_.scrollEnd;
					return;
                /* CSI <n> : <n> : <n> t -- window manipulation (xterm)

                   We do nothing for these at the moment, just recognize the few possibly interesting ones.
                 */
                case 't':
                    seq.setArgDefault(0, 0);
                    seq.setArgDefault(1, 0);
                    seq.setArgDefault(2, 0);
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
        }
        THROW(InvalidCSISequence(seq));
    }

    Color VT100::parseExtendedColor(CSISequence & seq, size_t & i) {
        ++i;
        if (i < seq.numArgs()) {
            switch (seq[i++]) {
                /* index from 256 colors */
                case 5:
                    if (i >= seq.numArgs()) // not enough args 
                        break;
                    if (seq[i] > 255) // invalid color spec
                        break;
                    return palette_[seq[i]];
                /* true color rgb */
                case 2:
                    i += 2;
                    if (i >= seq.numArgs()) // not enough args
                        break;
                    if (seq[i - 2] > 255 || seq[i - 1] > 255 || seq[i] > 255) // invalid color spec
                        break;
                    return Color(seq[i - 2], seq[i - 1], seq[i]);
                /* everything else is an error */
                default:
                    break;
            }
        }
        THROW(InvalidCSISequence("Invalid extended color: ", seq));
    }

    void VT100::parseSGR(CSISequence & seq) {
        seq.setArgDefault(0, 0);
        for (size_t i = 0; i < seq.numArgs(); ++i) {
            switch (seq[i]) {
                /* Resets all attributes. */
                case 0:
                    state_.font = Font();
                    state_.fg = palette_[defaultFg_];
                    state_.bg = palette_[defaultBg_];
                    LOG(SEQ) << "font fg bg reset";
                    break;
                /* Bold / bright foreground. */
                case 1:
                    state_.font.setBold(true);
                    LOG(SEQ) << "bold set";
                    break;
				/* Italics */
				case 3:
					state_.font.setItalics(true);
					LOG(SEQ) << "italics set";
					break;
                /* Underline */
                case 4:
                    state_.font.setUnderline(true);
                    LOG(SEQ) << "underline set";
                    break;
				/* Blinking text */
				case 5:
					state_.font.setBlink(true);
					LOG(SEQ) << "blink set";
					break;
				/* Strikeout */
				case 9:
					state_.font.setStrikeout(true);
					LOG(SEQ) << "strikeout";
					break;
                /* Normal - neither bold, nor faint. */
                case 22:
                    state_.font.setBold(false);
                    LOG(SEQ) << "normal font set";
                    break;
                /* Disable underline. */
                case 24:
                    state_.font.setUnderline(false);
                    LOG(SEQ) << "undeline unset";
                    break;
    			/* 30 - 37 are dark foreground colors, handled in the default case. */
    			/* 38 - extended foreground color */
                case 38:
                    state_.fg = parseExtendedColor(seq, i);
                    LOG(SEQ) << "fg set to " << state_.fg;
                    break;
                /* Foreground default. */
                case 39:
                    state_.fg = palette_[defaultFg_];
                    LOG(SEQ) << "fg reset";
                    break;
     			/* 40 - 47 are dark background color, handled in the default case. */
    			/* 48 - extended background color */
                case 48:
                    state_.bg = parseExtendedColor(seq, i);
                    LOG(SEQ) << "bg set to " << state_.bg;
                    break;
                /* Background default */
                case 49:
                    state_.bg = palette_[defaultBg_];
                    LOG(SEQ) << "bg reset";
                    break;

                /* 90 - 97 are bright foreground colors, handled in the default case. */
                /* 100 - 107 are bright background colors, handled in the default case. */

                default:
                    if (seq[i] >= 30 && seq[i] <= 37) {
                        state_.fg = palette_[seq[i] - 30];
                        LOG(SEQ) << "fg set to " << palette_[seq[i] - 30];
                    } else if (seq[i] >= 40 && seq[i] <= 47) {
                        state_.bg = palette_[seq[i] - 40];
                        LOG(SEQ) << "bg set to " << palette_[seq[i] - 40];
                    } else if (seq[i] >= 90 && seq[i] <= 97) {
                        state_.fg = palette_[seq[i] - 82];
                        LOG(SEQ) << "fg set to " << palette_[seq[i] - 82];
                    } else if (seq[i] >= 100 && seq[i] <= 107) {
                        state_.bg = palette_[seq[i] - 92];
                        LOG(SEQ) << "bg set to " << palette_[seq[i] - 92];
                    } else {
                        THROW(InvalidCSISequence("Invalid SGR code: ", seq));
                    }
                    break;
            }
        }
    }

    void VT100::parseSetterOrGetter(CSISequence & seq) {
        bool value = seq.finalByte() == 'h';
        for (size_t i = 0; i < seq.numArgs(); ++i) {
            int id = seq[i];
            switch (id) {
                /* application cursor mode on/off 
                 */
                case 1:
                    applicationCursorMode_ = value;
                    LOG(SEQ) << "application cursor mode: " << value;
                    continue;
                /* Smooth scrolling -- ignored*/
                case 4:
					LOG(SEQ_WONT_SUPPORT) << "Smooth scrolling: " << value;
                    continue;
				/* DECAWM - autowrap mode on/off */
				case 7:
					if (value)
						LOG(SEQ) << "autowrap mode enable (by default)";
					else
						LOG(SEQ_UNKNOWN) << "CSI?7l, DECAWM does not support being disabled";
					continue;
                // cursor blinking
                case 12:
                    cursor().blink = value;
                    LOG(SEQ) << "cursor blinking: " << value;
                    continue;
                // cursor show/hide
                case 25:
                    cursor().visible = value;
                    LOG(SEQ) << "cursor visible: " << value;
                    continue;
				/* Mouse tracking movement & buttons.

				https://stackoverflow.com/questions/5966903/how-to-get-mousemove-and-mouseclick-in-bash
				*/
				/* Enable normal mouse mode, i.e. report button press & release events only.
				 */
                case 1000:
					setMouseMode(value ? MouseMode::Normal : MouseMode::Off);
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
					setMouseMode(value ? MouseMode::ButtonEvent : MouseMode::Off);
					LOG(SEQ) << "button-event mouse tracking: " << value;
					continue;
				/* Report all mouse events (i.e. report mouse move even when buttons are not pressed).
				 */
                case 1003:
					setMouseMode(value ? MouseMode::All : MouseMode::Off);
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

				   TODO what is the difference between the two
                 */
                case 47: 
                case 1049:
                    if (value) {
                        if (! alternateBuffer_) {
                            storeCursorInfo();
                            flipBuffer();
                        }
                        resetCurrentBuffer();
                    } else {
                        if (alternateBuffer_) {
                            flipBuffer();
                            loadCursorInfo();
                        }
                    }
					invalidateAll_ = true;
                    continue;
                /* Enable/disable bracketed paste mode. When enabled, if user pastes code in the window, the contents should be enclosed with ESC [200~ and ESC[201~ so that the client app can determine it is contents of the clipboard (things like vi might otherwise want to interpret it. */
                case 2004:
                    bracketedPaste_ = value;
                    continue;
                default:
                    break;
            }
            THROW(InvalidCSISequence("Invalid Get/Set command: ", seq));
        }
    }

	void VT100::parseSaveOrRestore(CSISequence& seq) {
		for (size_t i = 0; i < seq.numArgs(); ++i)
			LOG(SEQ_WONT_SUPPORT) << "Private mode " << (seq.finalByte() == 's' ? "save" : "restore") << ", id " << seq[i];
	}

    /** Operating System Commands end with either an ST (ESC \), or with BEL. For now, only setting the terminal window command is supported. 
     */
	bool VT100::parseOSC() {
        // first try parsing
        char * start = buffer_;
        while (true) {
            // if we did not see the end of the sequence, we must read more
            if (eof())
                return false;
            if (condPop(Char::BEL))
                break;
            if (condPop(Char::ESC) && condPop('\\'))
                break;
            pop(); 
        }
        // we have now parsed the whole OSC - let's see if it is one we understand
		if (buffer_[-1] == Char::BEL && start[0] == '0' && start[1] == ';') {
			std::string title(start + 2, buffer_ - 1);
			LOG(SEQ) << "Title change to " << title;
			terminal()->setTitle(title);
		// set clipboard
		} else 	if (buffer_[-1] == Char::BEL && start[0] == '5' && start[1] == '2' && start[2] == ';') { 
			char* s = start + 3;
			while (*s != ';') {
				if (*s == Char::BEL) {
					LOG(SEQ_UNKNOWN) << "Unknown OSC: " << std::string(start, buffer_ - start);
					return true; // sequence matched, but unknown
				}
				++s;
			}
			++s; // the ';'
			std::string clipboard = helpers::Base64Decode(s, buffer_ - 1);
			LOG(SEQ) << "Setting clipboard to " << clipboard;
			setClipboard(clipboard);
        } else {
            // TODO ignore for now 112 == reset cursor color             
            LOG(SEQ_UNKNOWN) << "Unknown OSC: " << std::string(start, buffer_ - start);
        }
        return true;
	}


	void VT100::setCursor(unsigned col, unsigned row) {
		unsigned c = cols();
		while (col >= c) {
			col -= c;
			++row;
		}
		cursor().col = col;
		cursor().row = row;
	}


	void VT100::fillRect(helpers::Rect const & rect, Char::UTF8 c, Color fg, Color bg, Font font) {
		LOG(SEQ) << "fillRect (" << rect.left << "," << rect.top << "," << rect.right << "," << rect.bottom << ")  fg: " << fg << ", bg: " << bg;
		// TODO add print of the char as well
		for (unsigned row = rect.top; row < rect.bottom; ++row) {
			for (unsigned col = rect.left; col < rect.right; ++col) {
				Terminal::Cell & cell = buffer().at(col, row);
				cell.fg = fg;
				cell.bg = bg;
				cell.font = font;
				cell.c = c;
				cell.dirty = true;
			}
		}
	}

    void VT100::deleteLine(unsigned lines, unsigned from) {
        // don't do any scrolling if origin is outside scrolling region
        if (from < state_.scrollStart || from >= state_.scrollEnd)
            return;
        // delete the n lines by moving the lines below them up, be defensive on arguments
        lines = std::min(lines, state_.scrollEnd - from);
        for (unsigned r = from, re = state_.scrollEnd - lines; r < re; ++r) {
			for (unsigned c = 0; c < cols(); ++c) {
				Terminal::Cell & cell = buffer().at(c, r);
				cell = buffer().at(c, r + lines);
				cell.dirty = true;
			}
        }
        // now make the lines at the bottom empty
        for (unsigned r = state_.scrollEnd - lines; r < state_.scrollEnd; ++r) {
			for (unsigned c = 0; c < cols(); ++c) {
				Terminal::Cell & cell = buffer().at(c, r);
				cell.c = ' ';
				cell.fg = state_.fg;
				cell.bg = state_.bg;
				cell.font = Font();
				cell.dirty = true;
			}
        }
    }

    void VT100::insertLine(unsigned lines, unsigned from) {
		// don't do any scrolling if origin is outside scrolling region
        if (from < state_.scrollStart || from >= state_.scrollEnd)
            return;
        // create space by moving the contents in scroll window appropriate amount of lines down
        lines = std::min(lines, state_.scrollEnd - from);
        for (unsigned r = state_.scrollEnd - 1, rs = from + lines; r >= rs; --r) {
			for (unsigned c = 0; c < cols(); ++c) {
				Terminal::Cell & cell = buffer().at(c, r);
				cell = buffer().at(c, r - lines);
				cell.dirty = true;
			}
        }
        // now clear the top n lines
        for (unsigned r = from, re = from + lines; r < re; ++r) {
			for (unsigned c = 0; c < cols(); ++c) {
				Terminal::Cell & cell = buffer().at(c, r);
				cell.c = ' ';
				cell.fg = state_.fg;
				cell.bg = state_.bg;
				cell.font = Font();
				cell.dirty = true;
			}
        }
    }

	void VT100::deleteCharacters(unsigned num) {
		unsigned r = cursor().row;
		for (unsigned c = cursor().col, e = cols() - num; c < e; ++c) {
			Terminal::Cell& cell = buffer().at(c, r);
			cell = buffer().at(c + num, r);
			cell.dirty = true;
		}
		for (unsigned c = cols() - num, e = cols(); c < e; ++c) {
			Terminal::Cell& cell = buffer().at(c, r);
			cell.c = ' ';
			cell.fg = state_.fg;
			cell.bg = state_.bg;
			cell.font = Font();
			cell.dirty = true;
		}
	}

    void VT100::storeCursorInfo() {
        cursorStack_.push_back(cursor());
    }

    void VT100::loadCursorInfo() {
        ASSERT(! cursorStack_.empty());
        Terminal::Cursor const & ci = cursorStack_.back();
		cursor() = ci;
        if (cursor().col >= cols())
            cursor().col = cols() - 1;
        if (cursor().row >= rows())
            cursor().row = rows() - 1;
        cursorStack_.pop_back();
    }

	void VT100::flipBuffer() {
		alternateBuffer_ = ! alternateBuffer_;
        std::swap(state_, otherState_);
        std::swap(buffer(), otherBuffer_);
	}

    void VT100::resetCurrentBuffer() {
        state_.fg = palette_[defaultFg_];
        state_.bg = palette_[defaultBg_];
        state_.font = Font();
        fillRect(helpers::Rect(cols(), rows()), ' ');
        cursor() = Terminal::Cursor();
    }

	unsigned VT100::encodeMouseButton(MouseButton btn) {
		unsigned result =
			(inputState_.shift ? 4 : 0) +
			(inputState_.alt ? 8 : 0) +
			(inputState_.ctrl ? 16 : 0);
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

	void VT100::sendMouseEvent(unsigned button, unsigned col, unsigned row, char end) {
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
			sendData(buffer, 6);
			break;
		}
		case MouseEncoding::UTF8: {
			NOT_IMPLEMENTED;
			break;
		}
		case MouseEncoding::SGR: {
			std::string buffer = STR("\033[<" << button << ';' << col << ';' << row << end);
			sendData(buffer.c_str(), buffer.size());
			break;
		}
		}
	}



} // namespace vterm

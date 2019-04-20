#include <algorithm>
#include <iostream>


#include "helpers/strings.h"
#include "helpers/log.h"

#include "vt100.h"



namespace vterm {

	VT100::KeyMap VT100::keyMap_;


    // Palette

	Palette const Palette::Colors16{
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
		// add ctrl + letters
		for (unsigned k = 'A'; k <= 'Z'; ++k) {
			std::string seq;
			seq += static_cast<char>(k + 1 - 'A');
			addKey(Key(k) + Key::Ctrl, seq.c_str());
		}
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

    VT100::InvalidCSISequence::InvalidCSISequence(VT100::CSISequence const & seq):
        helpers::Exception(STR("Invalid CSI sequence: " << seq)) {
    }
    VT100::InvalidCSISequence::InvalidCSISequence(std::string const & msg, VT100::CSISequence const & seq):
        helpers::Exception(STR(msg << seq)) {
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
        if (! IsFinalByte(term.top()))
            return false;
        // parse the final byte
        finalByte_ = term.pop();
        end_ = term.buffer_;
        // if unsupported structure was encountered, throw the error
        if (throwAtEnd)
            THROW(InvalidCSISequence("Unsupported sequence structure :", *this));
        return true;
    }

    // VT100

	VT100::VT100(unsigned cols, unsigned rows, Palette const & palette, unsigned defaultFg, unsigned defaultBg) :
		IOTerminal(cols, rows),
		palette_(256),
		defaultFg_(defaultFg),
		defaultBg_(defaultBg),
		fg_(palette[defaultFg]),
		bg_(palette[defaultBg]),
		font_(),
	    buffer_(nullptr),
	    bufferEnd_(nullptr) {
		palette_.fillFrom(palette);
	}

	void VT100::keyDown(Key k) {
		std::string const* seq = keyMap_.getSequence(k);
		if (seq != nullptr) {
			write(seq->c_str(), seq->size());
			LOG << "key sent" << (unsigned) seq->c_str()[0] << " - " << seq->size();
		}
		LOG << "Key pressed: " << k;
	}

	void VT100::charInput(Char::UTF8 c) {
		// TODO make sure that the character is within acceptable range, i.e. it does not clash with ctrl+ stuff, etc. 
		write(c.rawBytes(), c.size());
	}

	void VT100::doResize(unsigned cols, unsigned rows) {
		// update the cursor so that it stays on the screen at all times
		if (cursorPos_.col >= cols_)
			cursorPos_.col = cols_ - 1;
		if (cursorPos_.row >= rows_)
			cursorPos_.row = rows_ - 1;
		// IOTerminal's doResize() is not called because of the virtual inheritance
		LOG(SEQ) << "terminal resized to " << cols << "," << rows;
	}

	void VT100::processInputStream(char * buffer, size_t & size) {
		buffer_ = buffer;
		bufferEnd_ = buffer_ + size;
		std::string text;
		{

			// lock the terminal for updates while decoding
			std::lock_guard<std::mutex> g(m_);

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
                    if (! parseEscapeSequence()) {
                        size = b - buffer;
                        return;
                    }
					break;
				}
				/* New line simply moves to next line.

				   TODO - should this also do carriage return? I guess so
				 */
				case Char::LF:
					LOG(SEQ) << "LF";
					pop();
					++cursorPos_.row;
					cursorPos_.col = 0;
					break;
				/* Carriage return sets cursor column to 0. 
				 */
				case Char::CR:
					LOG(SEQ) << "CR";
					pop();
					cursorPos_.col = 0;
					break;
				case Char::BACKSPACE: {
					if (cursorPos_.col == 0) {
						if (cursorPos_.row > 0)
							--cursorPos_.row;
						cursorPos_.col = cols_ - 1;
					} else {
						--cursorPos_.col;
					}
					// TODO backspace behaves weirdly - it seems we do not need to actually delete the character at all, in fact the whole backspace character seems to make very little difference
					//Cell& cell = defaultLayer_->at(cursorPos_.col, cursorPos_.row);
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
					if (! c8.readFromStream(buffer_, bufferEnd_)) {
						size = buffer_ - buffer;
						return;
					}
					// TODO fix this to utf8
					text += static_cast<char>(c8.codepoint() & 0xff);
					//LOG(SEQ) << "codepoint " << std::hex << c8.codepoint() << " " << static_cast<char>(c8.codepoint() & 0xff);
					// get the cell and update its contents
					Cell & cell = defaultLayer_->at(cursorPos_.col, cursorPos_.row);
					cell.fg = fg_;
					cell.bg = bg_;
					cell.font = font_;
					cell.c = c8;
					cell.dirty = true;
					// move to next column
					setCursor(cursorPos_.col + 1, cursorPos_.row);
				}
				}
			}
		}
		if (!text.empty()) {
			LOG(SEQ) << "text " << text;
			text.clear();
		}

		// TODO do not repaint all but only what has changed
		repaintAll();
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
                    LOG(UNKNOWN_SEQ) << e;
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
                LOG(UNKNOWN_SEQ) << "Unknown (possibly mismatched) character set final char " << pop();
                return true;
            /* Application keypad - TODO what is this */
            case '=':
                LOG(UNKNOWN_SEQ) << "Application keypad (ESC=)";
                return true;
            /* Normal keypad - TODO what is this */
            case '>':
                LOG(UNKNOWN_SEQ) << "Normal keypad (ESC>)";
                return true;
            /* Otherwise we have unknown escape sequence. This is an issue since we do not know when it ends and therefore may break the parsing.
             */
			default:
                LOG(UNKNOWN_SEQ) << "Unknown (possibly mismatched) char after ESC " << c;
				return true;
		}
	}

	void VT100::parseCSI(CSISequence & seq) {
        if (seq.firstByte() == '?') {
            switch (seq.finalByte()) {
                case 'h':
                case 'l':
                    if (seq.numArgs() == 1)
                        return parseSetterOrGetter(seq);
                    break;
                default:
                    break;
            }
        } else if (seq.firstByte() == 0) {
            switch (seq.finalByte()) {
                // CSI <n> A -- moves cursor n rows up 
                case 'A':
                    seq.setArgDefault(0, 1);
                    ASSERT(seq.numArgs() == 1);
                    LOG(SEQ) << "setCursor " << cursorPos_.col << ", " << cursorPos_.row - seq[0];
                    setCursor(cursorPos_.col, cursorPos_.row - seq[0]);
                    return;
                // CSI <n> B -- moves cursor n rows down 
                case 'B':
                    seq.setArgDefault(0, 1);
                    ASSERT(seq.numArgs() == 1);
                    LOG(SEQ) << "setCursor " << cursorPos_.col << ", " << cursorPos_.row + seq[0];
                    setCursor(cursorPos_.col, cursorPos_.row + seq[0]);
                    return;
                // CSI <n> C -- moves cursor n columns forward (right)
                case 'C':
                    seq.setArgDefault(0, 1);
                    ASSERT(seq.numArgs() == 1);
                    LOG(SEQ) << "setCursor " << cursorPos_.col + seq[0] << ", " << cursorPos_.row;
                    setCursor(cursorPos_.col + seq[0], cursorPos_.row);
                    return;
                // CSI <n> D -- moves cursor n columns back (left)
                case 'D': // cursor backward
                    seq.setArgDefault(0, 1);
                    ASSERT(seq.numArgs() == 1);
                    LOG(SEQ) << "setCursor " << cursorPos_.col - seq[0] << ", " << cursorPos_.row;
                    setCursor(cursorPos_.col - seq[0], cursorPos_.row);
                    return;
                /* set cursor coordinates */
                case 'H':
                case 'f':
                    seq.setArgDefault(0, 1);
                    seq.setArgDefault(1, 1);
                    ASSERT(seq.numArgs() == 2);
                    LOG(SEQ) << "setCursor " << seq[1] - 1 << ", " << seq[0] - 1;
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
                            fillRect(helpers::Rect(cursorPos_.col, cursorPos_.row, cols_, cursorPos_.row + 1), ' ');
                            fillRect(helpers::Rect(0, cursorPos_.row + 1, cols_, rows_), ' ');
                            return;
                        case 1:
                            updateCursorPosition();
                            fillRect(helpers::Rect(0, 0, cols_, cursorPos_.row), ' ');
                            fillRect(helpers::Rect(0, cursorPos_.row, cursorPos_.col + 1, cursorPos_.row + 1), ' ');
                            return;
                        case 2:
                            fillRect(helpers::Rect(cols_, rows_), ' ');
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
                            fillRect(helpers::Rect(cursorPos_.col, cursorPos_.row, cols_, cursorPos_.row + 1), ' ');
                            return;
                        case 1:
                            updateCursorPosition();
                            fillRect(helpers::Rect(0, cursorPos_.row, cursorPos_.col + 1, cursorPos_.row + 1), ' '); 
                            return;
                        case 2:
                            updateCursorPosition();
                            fillRect(helpers::Rect(0, cursorPos_.row, cols_, cursorPos_.row + 1), ' ');
                            return;
                        default:
                            break;
                    }
                    break;
                /* CSI <n> X -- erase <n> characters from the current position 
                */
                case 'X': {
                    seq.setArgDefault(0, 1);
                    ASSERT(seq.numArgs() == 1);
                    updateCursorPosition();
                    // erase from first line
                    unsigned n = static_cast<unsigned>(seq[0]);
                    unsigned l = std::min(cols_ - cursorPos_.col, n);
                    fillRect(helpers::Rect(cursorPos_.col, cursorPos_.row, cursorPos_.col + l, cursorPos_.row + 1), ' ');
                    n -= l;
                    // while there is enough stuff left to be larger than a line, erase entire line
                    l = cursorPos_.row + 1;
                    while (n >= cols_ && l < rows_) {
                        fillRect(helpers::Rect(0, l, cols_, l + 1), ' ');
                        ++l;
                        n -= cols_;
                    }
                    // if there is still something to erase, erase from the beginning
                    if (n != 0 && l < rows_) 
                        fillRect(helpers::Rect(0, l, n, l + 1), ' ');
                    return;
                }
                /* CSI <n> h -- Reset mode enable 
                Depending on the argument, certain things are turned on. None of the RM settings are currently supported. 
                */
                case 'h':
                    THROW(InvalidCSISequence("Unsupported CSI seqauence: ", seq));
                /* CSI <n> l -- Reset mode disable 
                Depending on the argument, certain things are turned off. Turning the features on/off is not allowed, but if the client wishes to disable something that is disabled, it's happily ignored. 
                */
                case 'l':
                    THROW(InvalidCSISequence("Unsupported CSI seqauence: ", seq));
                /* SGR 
                */
                case 'm':
                    return parseSGR(seq);
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
                case '5':
                    if (i == seq.numArgs()) // not enough args
                        break;
                    if (seq[i] > 255) // invalid color spec
                        break;
                    return palette_[seq[i]];
                /* true color rgb */
                case '2':
                    i += 2;
                    if (i >= seq.numArgs()) // not enough args
                        break;
                    if (seq[i - 2] > 255 || seq[i - 1] > 255 || seq[i] > 255) // invalid color spec
                        break;
                    return Color(seq[i - 2], seq[i - 1], seq[1]);
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
                    font_ = Font();
                    fg_ = defaultFg();
                    bg_ = defaultBg();
                    LOG(SEQ) << "font fg bg reset";
                    break;
                /* Bold / bright foreground. */
                case 1:
                    font_.setBold(true);
                    LOG(SEQ) << "bold set";
                    break;
                /* Underline */
                case 4:
                    font_.setUnderline(true);
                    LOG(SEQ) << "underline set";
                    break;
                /* Normal - neither bold, nor faint. */
                case 22:
                    font_.setBold(false);
                    LOG(SEQ) << "normal font set";
                    break;
                /* Disable underline. */
                case 24:
                    font_.setUnderline(false);
                    LOG(SEQ) << "undeline unset";
                    break;
    			/* 30 - 37 are dark foreground colors, handled in the default case. */
    			/* 38 - extended foreground color */
                case 38:
                    fg_ = parseExtendedColor(seq, i);
                    LOG(SEQ) << "fg set to " << fg_;
                    break;
                /* Foreground default. */
                case 39:
                    fg_ = defaultFg();
                    LOG(SEQ) << "fg reset";
                    break;
     			/* 40 - 47 are dark background color, handled in the default case. */
    			/* 48 - extended background color */
                case 48:
                    bg_ = parseExtendedColor(seq, i);
                    LOG(SEQ) << "fg set to " << fg_;
                    break;
                /* Background default */
                case 49:
                    bg_ = defaultBg();
                    LOG(SEQ) << "bg reset";
                    break;

                /* 90 - 97 are bright foreground colors, handled in the default case. */
                /* 100 - 107 are bright background colors, handled in the default case. */

                default:
                    if (seq[i] >= 30 && seq[i] <= 37) {
                        fg_ = palette_[seq[i] - 30];
                        LOG(SEQ) << "fg set to " << palette_[seq[i] - 30];
                    } else if (seq[i] >= 40 && seq[i] <= 47) {
                        bg_ = palette_[seq[i] - 40];
                        LOG(SEQ) << "bg set to " << palette_[seq[i] - 40];
                    } else if (seq[i] >= 90 && seq[i] <= 97) {
                        fg_ = palette_[seq[i] - 82];
                        LOG(SEQ) << "fg set to " << palette_[seq[i] - 82];
                    } else if (seq[i] >= 100 && seq[i] <= 107) {
                        bg_ = palette_[seq[i] - 92];
                        LOG(SEQ) << "bg set to " << palette_[seq[i] - 92];
                    } else {
                        THROW(InvalidCSISequence("Invalid SGR code: ", seq));
                    }
                    break;
            }
        }
    }

    void VT100::parseSetterOrGetter(CSISequence & seq) {
        int id = seq[0];
        bool value = seq.finalByte() == 'h';
		switch (id) {
			/* Smooth scrolling -- ignored*/
		    case 4:
				return;
			// cursor blinking
		    case 12:
				cursorBlink_ = value;
				LOG(SEQ) << "cursor blinking: " << value;
				return;
			// cursor show/hide
			case 25:
				cursorVisible_ = value;
				LOG(SEQ) << "cursor visible: " << value;
				return;
			/* Mouse tracking movement & buttons.

			   https://stackoverflow.com/questions/5966903/how-to-get-mousemove-and-mouseclick-in-bash
			 */
			case 1001: // mouse highlighting
			case 1006: // 
			case 1003:
                break;
			/* Enable or disable the alternate screen buffer.
			 */
			case 47: 
			case 1049:
				// switching to the alternate buffer, or enabling it again should trigger buffer reset
				fg_ = defaultFg();
				bg_ = defaultBg();
				fillRect(helpers::Rect(cols_, rows_), ' ');
				break;
			/* Enable/disable bracketed paste mode. When enabled, if user pastes code in the window, the contents should be enclosed with ESC [200~ and ESC[201~ so that the client app can determine it is contents of the clipboard (things like vi might otherwise want to interpret it. */
			case 2004:
				// TODO
				break;

    		default:
	    		break;
		}
        THROW(InvalidCSISequence("Invalid Get/Set command: ", seq));
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
            trigger(onTitleChange, title);
        } else {
            LOG(UNKNOWN_SEQ) << "Unknown OSC: " << std::string(start, buffer_ - start);
        }
        return true;
	}


	void VT100::setCursor(unsigned col, unsigned row) {
/*		while (col >= cols_) {
			col -= cols_;
			++row;
		} */
		cursorPos_.col = col;
		cursorPos_.row = row;
	}


	void VT100::fillRect(helpers::Rect const & rect, Char::UTF8 c, Color fg, Color bg, Font font) {
		LOG(SEQ) << "fillRect (" << rect.left << "," << rect.top << "," << rect.right << "," << rect.bottom << ")  fg: " << fg << ", bg: " << bg;
		// TODO add print of the char as well
		for (unsigned row = rect.top; row < rect.bottom; ++row) {
			for (unsigned col = rect.left; col < rect.right; ++col) {
				Cell & cell = defaultLayer_->at(col, row);
				cell.fg = fg;
				cell.bg = bg;
				cell.font = font;
				cell.c = c;
				cell.dirty = true;
			}
		}
	}

	void VT100::scrollDown(unsigned lines) {
		ASSERT(lines < rows_);
		LOG(SEQ) << "scrolling down " << lines << " lines";
		for (unsigned r = 0, re = rows_ - lines; r < re; ++r) {
			for (unsigned c = 0; c < cols_; ++c) {
				Cell & cell = defaultLayer_->at(c, r);
				cell = defaultLayer_->at(c, r + lines);
				cell.dirty = true;
			}
		}
		for (unsigned r = rows_ - lines; r < rows_; ++r) {
			for (unsigned c = 0; c < cols_; ++c) {
				Cell & cell = defaultLayer_->at(c, r);
				cell.c = ' ';
				cell.fg = fg_;
				cell.bg = bg_;
				cell.font = Font();
				cell.dirty = true;
			}
		}
	}

} // namespace vterm
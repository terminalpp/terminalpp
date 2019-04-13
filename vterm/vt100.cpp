#include <algorithm>
#include <iostream>


#include "helpers/strings.h"
#include "helpers/log.h"

#include "vt100.h"



namespace vterm {

	VT100::KeyMap VT100::keyMap_;

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


	VT100::VT100(unsigned cols, unsigned rows, Palette const & palette, unsigned defaultFg, unsigned defaultBg) :
		IOTerminal(cols, rows),
		palette_(256),
		defaultFg_(defaultFg),
		defaultBg_(defaultBg),
		fg_(this->defaultFg()),
		bg_(this->defaultBg()),
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
					if (!parseEscapeSequence()) {
						buffer_ = b;
						if (!skipEscapeSequence()) {
							size = b - buffer;
							return;
						} else {
							LOG(UNKNOWN_SEQ) << "ESC " << std::string(++b, buffer_);
						}
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

	bool VT100::parseEscapeSequence() {
		ASSERT(*buffer_ == Char::ESC);
		pop();
		switch (pop()) {
		    case ']':
				return parseOSC();
			case '[':
				return parseCSI();
			default:
				return false;
		}
	}

	bool VT100::parseCSI() {
		switch (top()) {
			// option setter
		    case '?':
				pop();
				return parseSetter();
			/* ESC [ H -- set cursor coordinates to top-left corner (default arg for row and col) 
			 */
			case 'H':
				pop();
				setCursor(0, 0);
				return true;
			/* ESC [ m -- SGR reset to default font and colors 
			 */
			case 'm':
				pop();
				return SGR(0);
			default:
				break;
		}
		// parse the first numeric argument
		unsigned first = parseNumber(1);
		switch (pop()) {
			// CSI <n> A -- moves cursor n rows up 
			case 'A':
				setCursor(cursorPos_.col, cursorPos_.row - first);
				return true;
			// CSI <n> B -- moves cursor n rows down 
			case 'B':
				setCursor(cursorPos_.col, cursorPos_.row + first);
				return true;
			// CSI <n> C -- moves cursor n columns forward (right)
			case 'C':
				setCursor(cursorPos_.col + first, cursorPos_.row);
				return true;
			// CSI <n> D -- moves cursor n columns back (left)
			case 'D': // cursor backward
				setCursor(cursorPos_.col - first, cursorPos_.row);
				return true;
			/* CSI <n> J -- erase display, depending on <n>:
			    0 = erase from the current position (inclusive) to the end of display
				1 = erase from the beginning to the current position(inclusive)
				2 = erase entire display
			 */
			case 'J':
				switch (first) {
					case 0:
						updateCursorPosition();
						fillRect(helpers::Rect(cursorPos_.col, cursorPos_.row, cols_, cursorPos_.row + 1), ' ');
						fillRect(helpers::Rect(0, cursorPos_.row + 1, cols_, rows_), ' ');
						return true;
					case 1:
						updateCursorPosition();
						fillRect(helpers::Rect(0, 0, cols_, cursorPos_.row), ' ');
						fillRect(helpers::Rect(0, cursorPos_.row, cursorPos_.col + 1, cursorPos_.row + 1), ' ');
						return true;
					case 2:
						fillRect(helpers::Rect(cols_, rows_), ' ');
						return true;
				    default:
						break;
				}
				return false;
			/* CSI <n> X -- erase <n> characters from the current position 
			 */
			case 'X': {
				updateCursorPosition();
				// erase from first line
				unsigned l = std::min(cols_ - cursorPos_.col, first);
				fillRect(helpers::Rect(cursorPos_.col, cursorPos_.row, cursorPos_.col + l, cursorPos_.row + 1), ' ');
				first -= l;
				// while there is enough stuff left to be larger than a line, erase entire line
				l = cursorPos_.row + 1;
				while (first >= cols_ && l < rows_) {
					fillRect(helpers::Rect(0, l, cols_, l + 1), ' ');
					++l;
					first -= cols_;
				}
				// if there is still something to erase, erase from the beginning
				if (first != 0 && l < rows_) 
					fillRect(helpers::Rect(0, l, first, l + 1), ' ');
				return true;
			}
			/* CSI <n> h -- Reset mode enable 
			   Depending on the argument, certain things are turned on. None of the RM settings are currently supported. 
			 */
			case 'h':
				return false;
			/* CSI <n> l -- Reset mode disable 
			   Depending on the argument, certain things are turned off. Turning the features on/off is not allowed, but if the client wishes to disable something that is disabled, it's happily ignored. 
			 */
			case 'l':
				switch (first) {
					/* CSI 4 l -- disable insert mode */
				    case 4:
						return true;
					default:
						return false;
				}
			/* CSI <n> m -- Set Graphics Rendition */
			case 'm': 
				return SGR(first);
			// second argument
			case ';':
				return parseCSI(first);
		    // otherwise we are dealing with an unrecognized sequence, return false
			default:
				return false;
		}
	}

	bool VT100::parseCSI(unsigned first) {
		unsigned second = parseNumber(1);
		switch (pop()) {
			// CSI <row> ; <col> H -- sets cursor position
			case 'H':
			case 'f':
				setCursor(second - 1, first - 1);
				return true;
				// third argument
			case ';': 
				return parseCSI(first, second);
			default:
				return false;
		}
	}

	bool VT100::parseCSI(unsigned first, unsigned second) {
		unsigned third = parseNumber(0);
		switch (pop()) {
			/* CSI 38 ; 5 ; <index> m - SGR 38, select from palette foreground */
			/* CSI 48 ; 5 ; <index> m - SGR 48, select from palette background */
		    case 'm': {
				if (second != 5 || third > 255)
					return false;
				if (first == 38) {
					fg_ = palette_[third];
					LOG(SEQ) << "fg set to index " << third << " (" << fg_ << ")";
				} else if (first == 48) {
					bg_ = palette_[third];
					LOG(SEQ) << "bg set to index " << third << " (" << bg_ << ")";
				} else {
					return false;
				}
				return true;
			}
			case ';':
				return parseCSI(first, second, third);
			default:
				return false;
		}
	}

	bool VT100::parseCSI(unsigned first, unsigned second, unsigned third) {
		unsigned fourth = parseNumber(0);
		switch (pop()) {
		    case ';':
				return parseCSI(first, second, third, fourth);
			default:
				return false;
		}
	}

	bool VT100::parseCSI(unsigned first, unsigned second, unsigned third, unsigned fourth) {
		unsigned fifth = parseNumber(0);
		switch (pop()) {
			/* CSI 38 ; 2 ; <r> ; <g> ; <b> m - SGR 38, specify color RGB */
			/* CSI 48 ; 2 ; <r> ; <g> ; <b> m - SGR 48, specify color RGB */
		    case 'm': {
				if (second != 2 || third > 255 || fourth > 255 || fifth > 255)
					return false;
				if (first == 38) {
					fg_ = Color(third, fourth, fifth);
					LOG(SEQ) << "fg set to " << fg_;
				} else if (first == 48) {
					bg_ = Color(third, fourth, fifth);
					LOG(SEQ) << "bg set to " << bg_;
				} else {
					return false;
				}
				return true;

			}
			default:
				return false;
		}
	}





	bool VT100::parseSetter() {
		int id = parseNumber();
		// the number must be followed by 'h' for turning on, or 'l' for turning the feature off
		bool value = condPop('h');
		if (! value && ! condPop('l'))
			return false;
		// TODO it can also be 's' for save
		switch (id) {
			/* Smooth scrolling -- ignored*/
		    case 4:
				return true;
			// cursor blinking
		    case 12:
				cursorBlink_ = value;
				LOG(SEQ) << "cursor blinking: " << value;
				return true;
			// cursor show/hide
			case 25:
				cursorVisible_ = value;
				LOG(SEQ) << "cursor visible: " << value;
				return true;
			/* Mouse tracking movement & buttons.

			   https://stackoverflow.com/questions/5966903/how-to-get-mousemove-and-mouseclick-in-bash
			 */
			case 1001: // mouse highlighting
			case 1006: // 
			case 1003:
				return false;
			/* Enable or disable the alternate screen buffer.
			 */
			case 47: 
			case 1049:
				// switching to the alternate buffer, or enabling it again should trigger buffer reset
				fg_ = defaultFg();
				bg_ = defaultBg();
				fillRect(helpers::Rect(cols_, rows_), ' ');
				return false;

			/* Enable/disable bracketed paste mode. When enabled, if user pastes code in the window, the contents should be enclosed with ESC [200~ and ESC[201~ so that the client app can determine it is contents of the clipboard (things like vi might otherwise want to interpret it. */
			case 2004:
				// TODO
				return true;

		default:
			return false;
		}
	}

	bool VT100::parseOSC() {
		unsigned id = parseNumber(0);
		switch (id) {
			/* ESC ] 0 ; <string> BEL -- Sets the window title. */
			case 0: {
				if (pop() != ';')
					return false;
				char * start = buffer_;
				while (!condPop(Char::BEL)) {
					if (eof()) // incomplete sequence
						return false;
					pop();
				}
				std::string title(start, buffer_);
				// trigger the event
				trigger(onTitleChange, title);
				return true;
			}
		    default:
				return false;
		}
	}

	bool VT100::SGR(unsigned value) {
		switch (value) {
			/* Resets all attributes. */
		    case 0:
				font_ = Font();
				fg_ = defaultFg();
				bg_ = defaultBg();
				LOG(SEQ) << "font fg bg reset";
				return true;
			/* Bold / bright foreground. */
			case 1:
				font_.setBold(true);
				LOG(SEQ) << "bold set";
				return true;
			/* Underline */
			case 4:
				font_.setUnderline(true);
				LOG(SEQ) << "underline set";
				return true;
			/* Normal - neither bold, nor faint. */
			case 22:
				font_.setBold(false);
				LOG(SEQ) << "normal font set";
				return true;
			/* Disable underline. */
			case 24:
				font_.setUnderline(false);
				LOG(SEQ) << "undeline unset";
				return true;
			/* 30 - 37 are dark foreground colors, handled in the default case. */
			/* 38 - extended foreground color - see parseCSI with more arguments */
			/* Foreground default. */
			case 39:
				fg_ = defaultFg();
				LOG(SEQ) << "fg reset";
				return true;
			/* 40 - 47 are dark background color, handled in the default case. */
			/* 38 - extended foreground color - see parseCSI with more arguments */
			/* Background default */
			case 49:
				bg_ = defaultBg();
				LOG(SEQ) << "bg reset";
				return true;

			/* 90 - 97 are bright foreground colors, handled in the default case. */
			/* 100 - 107 are bright background colors, handled in the default case. */

		    default:
				if (value >= 30 && value <= 37) {
					fg_ = palette_[value - 30];
					LOG(SEQ) << "fg set to " << palette_[value - 30];
				} else if (value >= 40 && value <= 47) {
					bg_ = palette_[value - 40];
					LOG(SEQ) << "bg set to " << palette_[value - 40];
				} else if (value >= 90 && value <= 97) {
					fg_ = palette_[value - 82];
					LOG(SEQ) << "fg set to " << palette_[value - 82];
				} else if (value >= 100 && value <= 107) {
					bg_ = palette_[value - 92];
					LOG(SEQ) << "bg set to " << palette_[value - 92];
				} else {
					return false;
				}
				return true;
		}
	}

	bool VT100::parseExtendedColor(Color& storeInto) {
		// already parsed ESC [ 38 or ESC [ 48
		if (!condPop(';'))
			return false;
		switch (parseNumber(0)) {
			case 2: {

			}
			case 5: {
				if (!condPop(';'))
					return false;


			}
			default:
				return false;
		}

	}

	void VT100::setCursor(unsigned col, unsigned row) {
		LOG(SEQ) << "setCursor " << col << ", " << row;
		while (col > cols_) {
			col -= cols_;
			++row;
		}
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
				defaultLayer_->at(c, r) = defaultLayer_->at(c, r + lines);
			}
		}
	}

} // namespace vterm
#include <algorithm>
#include <iostream>


#include "helpers/char.h"
#include "helpers/log.h"
#include "helpers/base64.h"

#include "vt100.h"

namespace vterm {

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
		auto& m = getModifierMap(k);
		auto i = m.find(k.code());
		if (i == m.end())
			return nullptr;
		else
			return &(i->second);
	}

	void VT100::KeyMap::addKey(Key k, char const* seq) {
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

	// VT100::CSISequence

	VT100::CSISequence::ParseResult VT100::CSISequence::parse(char *& start, char const* end) {
		ParseResult result = ParseResult::Valid;
		char * x = start;
		// if we are at the end, the sequence is incomplete
		if (x == end) 
			return ParseResult::Incomplete;
		// parse first byte
		if (IsParameterByte(*x) && (*x != ';') && !helpers::IsDecimalDigit(*x))
			firstByte_ = *x++;
		else
			firstByte_ = 0;
		// parse arguments, while any
		args_.clear();
		while (x != end && IsParameterByte(*x)) {
			if (*x == ';') {
				++x;
				args_.push_back(std::make_pair(0, false));
			} else if (helpers::IsDecimalDigit(*x)) {
				unsigned arg = 0;
				do {
					arg = arg * 10 + helpers::DecCharToNumber(*x++);
				} while (x != end && helpers::IsDecimalDigit(*x));
				args_.push_back(std::make_pair(arg, true));
				if (x != end && *x == ';')
					++x;
			} else {
				++x;
				result = ParseResult::Invalid;
			}
		}
		// parse intermediate bytes, which current implementation does not support
		while (x != end && IsIntermediateByte(*x)) {
			result = ParseResult::Invalid;
			++x;
		}
		// parse final byte, first check we are not at the end
		if (x == end)
			return ParseResult::Incomplete;
		if (IsFinalByte(*x))
			finalByte_ = *x++;
		else
			result = ParseResult::Invalid;
		// and we are done, depending on what we have in the result, return appropriately
		if (result == ParseResult::Invalid)
			LOG(SEQ_UNKNOWN) << "Unknown, possibly invalid CSI sequence: \x1b" << std::string(start - 1 , x - start + 1);
		start = x;
		return result;
	}

	// VT100

	VT100::KeyMap VT100::KeyMap_;

	Palette VT100::ColorsXTerm256() {
		Palette result(256);
		// start with the 16 colors
		result.fillFrom(Palette::Colors16());
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


	VT100::VT100(unsigned cols, unsigned rows,PTY* pty, size_t bufferSize) :
		PTYTerminal(cols, rows, pty, bufferSize),
		palette_(ColorsXTerm256()),
		defaultFg_(15),
		defaultBg_(0),
		state_(cols, rows),
		otherState_(cols, rows),
		otherScreen_(cols, rows),
		mouseMode_(MouseMode::Off),
		mouseEncoding_(MouseEncoding::Default),
		mouseLastButton_(0),
		alternateBuffer_(false),
		bracketedPaste_(false),
	    applicationCursorMode_(false),
	    applicationKeypadMode_(false) {
	}

	// Terminal input actions

	void VT100::keyDown(Key k) {
		inputState_.keyUpdate(k, true);
		std::string const* seq = KeyMap_.getSequence(k);
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
					ptyWrite(sa.c_str(), sa.size());
					return;
				}
				break;
			default:
				break;
			}
			ptyWrite(seq->c_str(), seq->size());
		}
	}

	void VT100::keyUp(Key k) {
		inputState_.keyUpdate(k, false);
	}

	void VT100::keyChar(helpers::Char c) {
		// TODO make sure that the character is within acceptable range, i.e. it does not clash with ctrl+ stuff, etc. 
		ptyWrite(c.toCharPtr(), c.size());
	}

	void VT100::mouseDown(unsigned col, unsigned row, MouseButton button) {
		inputState_.buttonUpdate(button, true);
		if (mouseMode_ == MouseMode::Off)
			return;
		mouseLastButton_ = encodeMouseButton(button);
		sendMouseEvent(mouseLastButton_, col, row, 'M');
		LOG(SEQ) << "Button " << button << " down at " << col << ";" << row;
	}

	void VT100::mouseUp(unsigned col, unsigned row, MouseButton button) {
		inputState_.buttonUpdate(button, false);
		if (mouseMode_ == MouseMode::Off)
			return;
		mouseLastButton_ = encodeMouseButton(button);
		sendMouseEvent(mouseLastButton_, col, row, 'm');
		LOG(SEQ) << "Button " << button << " up at " << col << ";" << row;
	}

	void VT100::mouseWheel(unsigned col, unsigned row, int by) {
		if (mouseMode_ == MouseMode::Off)
			return;
		// mouse wheel adds 64 to the value
		mouseLastButton_ = encodeMouseButton((by > 0) ? MouseButton::Left : MouseButton::Right) + 64;
		sendMouseEvent(mouseLastButton_, col, row, 'M');
		LOG(SEQ) << "Wheel offset " << by << " at " << col << ";" << row;
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
	
	void VT100::paste(std::string const& what) {
		if (bracketedPaste_) {
			ptyWrite("\033[200~", 6);
			ptyWrite(what.c_str(), what.size());
			ptyWrite("\033[201~", 6);
		} else {
			ptyWrite(what.c_str(), what.size());
		}
	}

	size_t VT100::doProcessInput(char* buffer, size_t size) {
		{
			Terminal::ScreenLock sl = lockScreen();
			char* bufferEnd = buffer + size;
			char* x = buffer;
			CSISequence seq;
			while (x != bufferEnd) {
				switch (*x) {
					case helpers::Char::ESC: {
						if (!parseEscapeSequence(x, bufferEnd))
							return x - buffer;
						break;
					}
					/* BEL is sent if user should be notified, i.e. play a notification sound or so.
					 */
					case helpers::Char::BEL:
						++x;
						LOG(SEQ) << "BEL notification";
						// TODO should the lock be released here? 
						trigger(onNotification);
						break;
					case helpers::Char::TAB:
						// TODO tabstops and stuff
						++x;
						updateCursorPosition();
						if (screen_.cursor().col % 8 == 0)
							screen_.cursor().col += 8;
						else
							screen_.cursor().col += 8 - (screen_.cursor().col % 8);
						LOG(SEQ) << "Tab: cursor col is " << screen_.cursor().col;
						break;
					/* New line simply moves to next line.
					 */
					case helpers::Char::LF:
						LOG(SEQ) << "LF";
						markLastCharPosition();
						++x;
						++screen_.cursor().row;
						updateCursorPosition();
						setLastCharPosition();
						break;
					/* Carriage return sets cursor column to 0.
					 */
					case helpers::Char::CR:
						LOG(SEQ) << "CR";
						markLastCharPosition();
						++x;
						screen_.cursor().col = 0;
						break;
					case helpers::Char::BACKSPACE: {
						LOG(SEQ) << "BACKSPACE";
						++x;
						if (screen_.cursor().col == 0) {
							if (screen_.cursor().row > 0)
								--screen_.cursor().row;
							screen_.cursor().col = screen_.cols() - 1;
						}
						else {
							--screen_.cursor().col;
						}
						break;
					}
  					/* default variant is to print the character received to current cell.
					 */
					default: {
						// make sure that the cursor is in visible part of the screen
						updateCursorPosition();
						// it could be we are dealing with unicode
						helpers::Char const * c8 = helpers::Char::At(x, bufferEnd);
						if (c8 == nullptr)
							return x - buffer;
						//LOG << "codepoint " << std::hex << c8->codepoint() << " " << static_cast<char>(c8->codepoint() & 0xff);
						// get the cell and update its contents
						Terminal::Cell& cell = screen_.at(screen_.cursor().col, screen_.cursor().row);
						cell.setFg(state_.fg);
						cell.setBg(state_.bg);
						cell.setFont(state_.font);
						cell.setC(*c8);
						// store the last character position
						setLastCharPosition();
						// move to next column
						++screen_.cursor().col;
					}
				}
			}
		}
		trigger(onRepaint);
		return size;
	}

	bool VT100::parseEscapeSequence(char*& buffer, char* bufferEnd) {
		ASSERT(*buffer == helpers::Char::ESC);
		char* x = buffer;
		++x;
		// if at eof now, we need to read more so that we know what to escape
		if (x == bufferEnd)
			return false;
		switch (*x++) {
			/* Reverse line feed - move up 1 row, same column.
			 */
			case 'M':
				LOG(SEQ) << "RI: move cursor 1 line up";
				if (screen_.cursor().row == 0) {
					insertLine(1, 0);
				} else {
					setCursor(screen_.cursor().col, screen_.cursor().row - 1);
				}
				break;
			/* Operating system command. */
			case ']':
				if (!parseOSCSequence(x, bufferEnd))
					return false;
				break;
		    /* CSI Sequence - parse using the CSISequence class. */
			case '[': {
				switch (seq_.parse(x, bufferEnd)) {
				case CSISequence::ParseResult::Valid:
					processCSISequence();
				case CSISequence::ParseResult::Invalid:
					break;
				case CSISequence::ParseResult::Incomplete:
					return false;
				}
				break;
			}
    		/* Character set specification - ignored, we just have to parse it. */
			case '(':
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
				applicationKeypadMode_ = true;
				break;
			/* ESC > -- Normal keypad */
			case '>':
				LOG(SEQ) << "Normal keypad mode enabled";
				applicationKeypadMode_ = false;
				break;
			/* Otherwise we have unknown escape sequence. This is an issue since we do not know when it ends and therefore may break the parsing.
			 */
			default:
				LOG(SEQ_UNKNOWN) << "Unknown (possibly mismatched) char after ESC " << *x;
				++x;
				break;
		}
		buffer = x;
		return true;
	}

	bool VT100::parseOSCSequence(char*& buffer, char* end) {
		// first find the end of the sequence, to determine whether it is complete or not
		char* x = buffer;
		while (true) {
			// if we did not see the end of the sequence, we must read more
			if (x == end)
				return false;
			// by default, the OSC ends with BEL character
			if (*x == helpers::Char::BEL)
				break;
			if (*x == helpers::Char::ESC) {
				++x;
				if (x == end)
					return false;
				if (*x == '\\')
					break;
			}
			++x;
		}
		++x;
		// update the buffer since the OSC sequence has been parsed properly
		char* start = buffer;
		buffer = x;
		// we have now parsed the whole OSC - let's see if it is one we understand
		if (x[-1] == helpers::Char::BEL && start[0] == '0' && start[1] == ';') {
			std::string title(start + 2, x - 1);
			LOG(SEQ) << "Title change to " << title;
			// TODO release the lock? 
			setTitle(title);
		// set clipboard
		} else if (x[-1] == helpers::Char::BEL && start[0] == '5' && start[1] == '2' && start[2] == ';') {
			char* s = buffer + 3;
			while (*s != ';') {
				if (*s == helpers::Char::BEL) {
					LOG(SEQ_UNKNOWN) << "Unknown OSC: " << std::string(start, x - start);
					return true; // sequence matched, but unknown
				}
				++s;
			}
			++s; // the ';'
			std::string clipboard = helpers::Base64Decode(s, x - 1);
			LOG(SEQ) << "Setting clipboard to " << clipboard;
			// TODO release the lock? 
			trigger(onClipboardUpdated, clipboard);
		} else {
			// TODO ignore for now 112 == reset cursor color             
			LOG(SEQ_UNKNOWN) << "Unknown OSC: " << std::string(start, x - start);
		}
		return true;
	}

	void VT100::processCSISequence() {
		if (seq_.firstByte() == '?') {
			switch (seq_.finalByte()) {
				/* Multiple options can be set high or low in a single command.
				 */
				case 'h':
				case 'l':
					return processSetterOrGetter();
				case 's':
				case 'r':
					return processSaveOrRestore();
				default:
					break;
			}
		} else if (seq_.firstByte() == '>') {
			switch (seq_.finalByte()) {
				/* CSI > 0 c -- Send secondary device attributes.
				 */
				case 'c':
					if (seq_[0] != 0)
						break;
					LOG(SEQ) << "Secondary Device Attributes - VT100 sent";
					ptyWrite("\033[>0;0;0c", 5); // we are VT100, no version third must always be zero (ROM cartridge)
					return;
				default:
					break;
			}
		} else if (seq_.firstByte() == 0) {
			switch (seq_.finalByte()) {
				// CSI <n> @ -- insert blank characters (ICH)
				case '@':
					seq_.setArgDefault(0, 1);
					LOG(SEQ) << "ICH: deleteCharacter " << seq_[0];
					insertCharacters(seq_[0]);
					return;
				// CSI <n> A -- moves cursor n rows up (CUU)
				case 'A': {
					seq_.setArgDefault(0, 1);
					ASSERT(seq_.numArgs() == 1);
					unsigned r = screen_.cursor().row >= seq_[0] ? screen_.cursor().row - seq_[0] : 0;
					LOG(SEQ) << "CUU: setCursor " << screen_.cursor().col << ", " << r;
					setCursor(screen_.cursor().col, r);
					return;
				}
				// CSI <n> B -- moves cursor n rows down (CUD)
				case 'B':
					seq_.setArgDefault(0, 1);
					ASSERT(seq_.numArgs() == 1);
					LOG(SEQ) << "CUD: setCursor " << screen_.cursor().col << ", " << screen_.cursor().row + seq_[0];
					setCursor(screen_.cursor().col, screen_.cursor().row + seq_[0]);
					return;
				// CSI <n> C -- moves cursor n columns forward (right) (CUF)
				case 'C':
					seq_.setArgDefault(0, 1);
					ASSERT(seq_.numArgs() == 1);
					LOG(SEQ) << "CUF: setCursor " << screen_.cursor().col + seq_[0] << ", " << screen_.cursor().row;
					setCursor(screen_.cursor().col + seq_[0], screen_.cursor().row);
					return;
				// CSI <n> D -- moves cursor n columns back (left) (CUB)
				case 'D': {// cursor backward
					seq_.setArgDefault(0, 1);
					ASSERT(seq_.numArgs() == 1);
					unsigned c = screen_.cursor().col >= seq_[0] ? screen_.cursor().col - seq_[0] : 0;
					LOG(SEQ) << "CUB: setCursor " << c << ", " << screen_.cursor().row;
					setCursor(c, screen_.cursor().row);
					return;
				}
				/* CSI <n> G -- set cursor character absolute (CHA)
				 */
				case 'G':
					seq_.setArgDefault(0, 1);
					LOG(SEQ) << "CHA: set column " << seq_[0] - 1;
					setCursor(seq_[0] - 1, screen_.cursor().row);
					return;
				/* set cursor position (CUP) */
				case 'H': // CUP
				case 'f': // HVP
					seq_.setArgDefault(0, 1);
					seq_.setArgDefault(1, 1);
					ASSERT(seq_.numArgs() == 2);
					LOG(SEQ) << "CUP: setCursor " << seq_[1] - 1 << ", " << seq_[0] - 1;
					setCursor(seq_[1] - 1, seq_[0] - 1);
					return;
				/* CSI <n> J -- erase display, depending on <n>:
					0 = erase from the current position (inclusive) to the end of display
					1 = erase from the beginning to the current position(inclusive)
					2 = erase entire display
				*/
				case 'J':
					ASSERT(seq_.numArgs() <= 1);
					switch (seq_[0]) {
						case 0:
							updateCursorPosition();
							fillRect(helpers::Rect(screen_.cursor().col, screen_.cursor().row, screen_.cols(), screen_.cursor().row + 1), ' ');
							fillRect(helpers::Rect(0, screen_.cursor().row + 1, screen_.cols(), screen_.rows()), ' ');
							return;
						case 1:
							updateCursorPosition();
							fillRect(helpers::Rect(0, 0, screen_.cols(), screen_.cursor().row), ' ');
							fillRect(helpers::Rect(0, screen_.cursor().row, screen_.cursor().col + 1, screen_.cursor().row + 1), ' ');
							return;
						case 2:
							fillRect(helpers::Rect(screen_.cols(), screen_.rows()), ' ');
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
					ASSERT(seq_.numArgs() <= 1);
					switch (seq_[0]) {
						case 0:
							updateCursorPosition();
							fillRect(helpers::Rect(screen_.cursor().col, screen_.cursor().row, screen_.cols(), screen_.cursor().row + 1), ' ');
							return;
						case 1:
							updateCursorPosition();
							fillRect(helpers::Rect(0, screen_.cursor().row, screen_.cursor().col + 1, screen_.cursor().row + 1), ' ');
							return;
						case 2:
							updateCursorPosition();
							fillRect(helpers::Rect(0, screen_.cursor().row, screen_.cols(), screen_.cursor().row + 1), ' ');
							return;
						default:
							break;
					}
					break;
				/* CSI <n> L -- Insert n lines. (IL)
				 */
				case 'L':
					seq_.setArgDefault(0, 1);
					LOG(SEQ) << "IL: scrollUp " << seq_[0];
					insertLine(seq_[0], screen_.cursor().row);
					return;
				/* CSI <n> M -- Remove n lines. (DL)
				 */
				case 'M':
					seq_.setArgDefault(0, 1);
					LOG(SEQ) << "DL: scrollDown " << seq_[0];
					deleteLine(seq_[0], screen_.cursor().row);
					return;
				/* CSI <n> P -- Delete n charcters. (DCH) */
				case 'P':
					seq_.setArgDefault(0, 1);
					LOG(SEQ) << "DCH: deleteCharacter " << seq_[0];
					deleteCharacters(seq_[0]);
					return;
				/* CSI <n> S -- Scroll up n lines
				 */
				case 'S':
					seq_.setArgDefault(0, 1);
					LOG(SEQ) << "SU: scrollUp " << seq_[0];
					deleteLine(seq_[0], state_.scrollStart);
					return;
				/* CSI <n> T -- Scroll down n lines
				 */
				case 'T':
					seq_.setArgDefault(0, 1);
					LOG(SEQ) << "SD: scrollDown " << seq_[0];
					// TODO should this be from cursor, or from scrollStart? 
					insertLine(seq_[0], screen_.cursor().row);
					return;
				/* CSI <n> X -- erase <n> characters from the current position
				*/
				case 'X': {
					seq_.setArgDefault(0, 1);
					ASSERT(seq_.numArgs() == 1);
					updateCursorPosition();
					// erase from first line
					unsigned n = static_cast<unsigned>(seq_[0]);
					unsigned l = std::min(screen_.cols() - screen_.cursor().col, n);
					fillRect(helpers::Rect(screen_.cursor().col, screen_.cursor().row, screen_.cursor().col + l, screen_.cursor().row + 1), ' ');
					n -= l;
					// while there is enough stuff left to be larger than a line, erase entire line
					l = screen_.cursor().row + 1;
					while (n >= screen_.cols() && l < screen_.rows()) {
						fillRect(helpers::Rect(0, l, screen_.cols(), l + 1), ' ');
						++l;
						n -= screen_.cols();
					}
					// if there is still something to erase, erase from the beginning
					if (n != 0 && l < screen_.rows())
						fillRect(helpers::Rect(0, l, n, l + 1), ' ');
					return;
				}
  			    /* CSI <n> c - primary device attributes.
				 */
				case 'c': {
					if (seq_[0] != 0)
						break;
					LOG(SEQ) << "Device Attributes - VT102 sent";
					ptyWrite("\033[?6c", 5); // send VT-102 for now, go for VT-220? 
					return;
				}
				/* CSI <n> d -- Line position absolute (VPA)
				 */
				case 'd': {
					seq_.setArgDefault(0, 1);
					if (seq_.numArgs() != 1)
						break;
					unsigned r = seq_[0];
					if (r < 1)
						r = 1;
					else if (r > screen_.rows())
						r = screen_.rows();
					LOG(SEQ) << "VPA: setCursor " << screen_.cursor().col << ", " << r - 1;
					setCursor(screen_.cursor().col, r - 1);
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
					seq_.setArgDefault(0, 0);
					// enable replace mode (IRM) since this is the only mode we allow, do nothing
					if (seq_[0] == 4)
						return;
					break;
				/* SGR
				 */
				case 'm':
					return processSGR();
				/* CSI <n> ; <n> r -- Set scrolling region (default is the whole window)
				 */
				case 'r':
					seq_.setArgDefault(0, 1); // inclusive
					seq_.setArgDefault(1, screen_.rows()); // inclusive
					if (seq_.numArgs() != 2)
						break;
					state_.scrollStart = std::min(seq_[0] - 1, screen_.rows() - 1); // inclusive
					state_.scrollEnd = std::min(seq_[1], screen_.rows()); // exclusive 
					LOG(SEQ) << "Scroll region set to " << state_.scrollStart << " - " << state_.scrollEnd;
					return;
				/* CSI <n> : <n> : <n> t -- window manipulation (xterm)

					We do nothing for these at the moment, just recognize the few possibly interesting ones.
     			 */
				case 't':
					seq_.setArgDefault(0, 0);
					seq_.setArgDefault(1, 0);
					seq_.setArgDefault(2, 0);
					switch (seq_[0]) {
					case 22:
						// 22;0;0 -- save xterm icon and window title on stack
						if (seq_[1] == 0 && seq_[2] == 0)
							return;
						break;
					case 23:
						// 23;0;0 -- restore xterm icon and window title from stack
						if (seq_[1] == 0 && seq_[2] == 0)
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
		LOG(SEQ_UNKNOWN) << " Unknown CSI sequence " << seq_;
	}

	void VT100::processSetterOrGetter() {
		bool value = seq_.finalByte() == 'h';
		for (size_t i = 0; i < seq_.numArgs(); ++i) {
			int id = seq_[i];
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
					screen_.cursor().blink = value;
					LOG(SEQ) << "cursor blinking: " << value;
					continue;
				// cursor show/hide
				case 25:
					screen_.cursor().visible = value;
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
				 */
				case 47:
				case 1049:
					if (value) {
						// flip to alternate buffer and clear it
						if (!alternateBuffer_) {
							otherScreen_ = screen_;
							std::swap(state_, otherState_);
							invalidateLastCharPosition();
						}
						state_.fg = palette_[defaultFg_];
						state_.bg = palette_[defaultBg_];
						state_.font = Font();
						fillRect(helpers::Rect(screen_.cols(), screen_.rows()), ' ');
						screen_.cursor() = Terminal::Cursor();
						LOG(SEQ) << "Alternate screen on";
					} else {
						// go back from alternate buffer
						if (alternateBuffer_) {
							screen_ = otherScreen_;
							std::swap(state_, otherState_);
							screen_.markDirty();
						}
						LOG(SEQ) << "Alternate screen off";
					}
					alternateBuffer_ = value;
					continue;
				/* Enable/disable bracketed paste mode. When enabled, if user pastes code in the window, the contents should be enclosed with ESC [200~ and ESC[201~ so that the client app can determine it is contents of the clipboard (things like vi might otherwise want to interpret it. 
				 */
				case 2004:
					bracketedPaste_ = value;
					continue;
				default:
					break;
			}
			LOG(SEQ_UNKNOWN) << "Invalid Get/Set command: " << seq_;
		}
	}

	void VT100::processSaveOrRestore() {
		for (size_t i = 0; i < seq_.numArgs(); ++i)
			LOG(SEQ_WONT_SUPPORT) << "Private mode " << (seq_.finalByte() == 's' ? "save" : "restore") << ", id " << seq_[i];
	}

	void VT100::processSGR() {
		seq_.setArgDefault(0, 0);
		for (size_t i = 0; i < seq_.numArgs(); ++i) {
			switch (seq_[i]) {
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
					/* faint font (light) - won't support for now, though in theory we easily can. */
				case 2:
					LOG(SEQ_WONT_SUPPORT) << "faint font";
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
					/* Inverse and inverse off */
				case 7:
				case 27:
					std::swap(state_.fg, state_.bg);
					LOG(SEQ) << "toggle inverse mode";
					break;
					/* Strikethrough */
				case 9:
					state_.font.setStrikethrough(true);
					LOG(SEQ) << "strikethrough";
					break;
					/* Bold off */
				case 21:
					state_.font.setBold(false);
					LOG(SEQ) << "bold off";
					break;
					/* Normal - neither bold, nor faint. */
				case 22:
					state_.font.setBold(false);
					LOG(SEQ) << "normal font set";
					break;
					/* Italics off. */
				case 23:
					state_.font.setItalics(false);
					LOG(SEQ) << "italics off";
					break;
					/* Disable underline. */
				case 24:
					state_.font.setUnderline(false);
					LOG(SEQ) << "undeline off";
					break;
					/* Disable blinking. */
				case 25:
					state_.font.setBlink(false);
					LOG(SEQ) << "blink off";
					break;
					/* Disable strikethrough. */
				case 29:
					state_.font.setStrikethrough(false);
					LOG(SEQ) << "Strikethrough off";
					break;
					/* 30 - 37 are dark foreground colors, handled in the default case. */
					/* 38 - extended foreground color */
				case 38:
					state_.fg = processSGRExtendedColor(i);
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
					state_.bg = processSGRExtendedColor(i);
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
					if (seq_[i] >= 30 && seq_[i] <= 37) {
						state_.fg = palette_[seq_[i] - 30];
						LOG(SEQ) << "fg set to " << palette_[seq_[i] - 30];
					} else if (seq_[i] >= 40 && seq_[i] <= 47) {
						state_.bg = palette_[seq_[i] - 40];
						LOG(SEQ) << "bg set to " << palette_[seq_[i] - 40];
					} else if (seq_[i] >= 90 && seq_[i] <= 97) {
						state_.fg = palette_[seq_[i] - 82];
						LOG(SEQ) << "fg set to " << palette_[seq_[i] - 82];
					} else if (seq_[i] >= 100 && seq_[i] <= 107) {
						state_.bg = palette_[seq_[i] - 92];
						LOG(SEQ) << "bg set to " << palette_[seq_[i] - 92];
					} else {
						LOG(SEQ_UNKNOWN) << "Invalid SGR code: " << seq_;
					}
					break;
			}
		}
	}

	Color VT100::processSGRExtendedColor(size_t& i) {
		++i;
		if (i < seq_.numArgs()) {
			switch (seq_[i++]) {
				/* index from 256 colors */
				case 5:
					if (i >= seq_.numArgs()) // not enough args 
						break;
					if (seq_[i] > 255) // invalid color spec
						break;
					return palette_[seq_[i]];
				/* true color rgb */
				case 2:
					i += 2;
					if (i >= seq_.numArgs()) // not enough args
						break;
					if (seq_[i - 2] > 255 || seq_[i - 1] > 255 || seq_[i] > 255) // invalid color spec
						break;
					return Color(seq_[i - 2], seq_[i - 1], seq_[i]);
				/* everything else is an error */
				default:
					break;
			}
		}
		LOG(SEQ_UNKNOWN) << "Invalid extended color: " << seq_;
		return Color::White();
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
				ptyWrite(buffer, 6);
				break;
			}
			case MouseEncoding::UTF8: {
				NOT_IMPLEMENTED;
				break;
			}
			case MouseEncoding::SGR: {
				std::string buffer = STR("\033[<" << button << ';' << col << ';' << row << end);
				ptyWrite(buffer.c_str(), buffer.size());
				break;
			}
		}
	}

	void VT100::setCursor(unsigned col, unsigned row) {
		screen_.cursor().col = col;
		screen_.cursor().row = row;
		// invalidate the last character position
		invalidateLastCharPosition();
	}

	void VT100::fillRect(helpers::Rect const& rect, helpers::Char c, Color fg, Color bg, Font font) {
		LOG(SEQ) << "fillRect (" << rect.left << "," << rect.top << "," << rect.right << "," << rect.bottom << ")  fg: " << fg << ", bg: " << bg << ", character: " << c;
		for (unsigned row = rect.top; row < rect.bottom; ++row) {
			for (unsigned col = rect.left; col < rect.right; ++col) {
				Terminal::Cell& cell = screen_.at(col, row);
				cell.setFg(fg);
				cell.setBg(bg);
				cell.setFont(font);
				cell.setC(c);
			}
		}
	}

	void VT100::deleteLine(unsigned lines, unsigned from) {
        Cell c;
        c.setFg(state_.fg);
        c.setBg(state_.bg);
        c.setFont(Font());
        c.setC(' ');
        screen_.deleteLines(lines, from, state_.scrollEnd, c);
	}

	void VT100::insertLine(unsigned lines, unsigned from) {
        Cell c;
        c.setFg(state_.fg);
        c.setBg(state_.bg);
        c.setFont(Font());
        c.setC(' ');
        screen_.insertLines(lines, from, state_.scrollEnd, c);
	}

	void VT100::deleteCharacters(unsigned num) {
		unsigned r = screen_.cursor().row;
		for (unsigned c = screen_.cursor().col, e = screen_.cols() - num; c < e; ++c) {
			Terminal::Cell& cell = screen_.at(c, r);
			cell = screen_.at(c + num, r);
		}
		for (unsigned c = screen_.cols() - num, e = screen_.cols(); c < e; ++c) {
			Terminal::Cell& cell = screen_.at(c, r);
			cell.setC(' ');
			cell.setFg(state_.fg);
			cell.setBg(state_.bg);
			cell.setFont(Font());
		}
	}

	void VT100::insertCharacters(unsigned num) {
		unsigned r = screen_.cursor().row;
		// first copy the characters
		for (unsigned c = screen_.cols() - 1, e = screen_.cursor().col + num; c >= e; --c) {
			Terminal::Cell& cell = screen_.at(c, r);
			cell = screen_.at(c - num, r);
		}
		for (unsigned c = screen_.cursor().col, e = screen_.cursor().col + num; c < e; ++c) {
			Terminal::Cell& cell = screen_.at(c, r);
			cell.setC(' ');
			cell.setFg(state_.fg);
			cell.setBg(state_.bg);
			cell.setFont(Font());
		}
	}

} // namespace vterm

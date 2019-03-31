#include <algorithm>
#include <iostream>


#include "helpers/strings.h"

#include "vt100.h"

namespace vterm {

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

	VT100::VT100(unsigned cols, unsigned rows, Palette const & palette, unsigned defaultFg, unsigned defaultBg) :
		IOTerminal(cols, rows),
		palette_(256),
		defaultFg_(defaultFg),
		defaultBg_(defaultBg),
		fg_(this->defaultFg()),
		bg_(this->defaultBg()),
		font_(),
		cursorCol_(0),
        cursorRow_(0) {
		palette_.fillFrom(palette);
	}

	void VT100::doResize(unsigned cols, unsigned rows) {
		// update the cursor so that it stays on the screen at all times
		if (cursorCol_ >= cols_)
			cursorCol_ = cols_ - 1;
		if (cursorRow_ >= rows_)
			cursorRow_ = rows_ - 1;
		// IOTerminal's doResize() is not called because of the virtual inheritance
	}


	void VT100::processInputStream(char * buffer, size_t & size) {
		buffer_ = buffer;
		bufferEnd_ = buffer_ + size;
		{

			// lock the terminal for updates while decoding
			std::lock_guard<std::mutex> g(m_);

			while (! eof()) {
				char c = top();
				switch (c) {
				case Char::ESC: {
					// make a copy of buffer in case process escape sequence advances it and then fails
					char * b = buffer_;
					if (!parseEscapeSequence()) {
						buffer_ = b;
						if (!skipEscapeSequence()) {
							size = b - buffer;
							return;
						} else {
							std::cout << "unknown escape sequence: ESC ";
							for (++b; b < buffer_; ++b)
								std::cout << *b;
							std::cout << std::endl;
						}
					}
					break;
				}
				/* New line simply moves to next line.

				   TODO - should this also do carriage return? I guess so
				 */
				case Char::LF:
					pop();
					++cursorRow_;
					cursorCol_ = 0;
					break;
				/* Carriage return sets cursor column to 0. 
				 */
				case Char::CR:
					pop();
					cursorCol_ = 0;
					break;
				case Char::BACKSPACE:
					pop();
					// TODO
					break;
				/* default variant is to print the character received to current cell.
				 */
				default: {
					// make sure that cursor is positioned within the terminal (previous advances may have placed it outside of the terminal area)
					updateCursorPosition();
					// it could be we are dealing with unicode
					Char::UTF8 c8;
					if (! c8.readFromStream(buffer_, bufferEnd_)) {
						size = buffer_ - buffer;
						return;
					}
					// get the cell and update its contents
					Cell & cell = defaultLayer_->at(cursorCol_, cursorRow_);
					cell.fg = fg_;
					cell.bg = bg_;
					cell.font = font_;
					cell.c = c8;
					// move to next column
					++cursorCol_;
				}
				}
			}
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
				setCursor(cursorCol_, cursorRow_ - first);
				return true;
			// CSI <n> B -- moves cursor n rows down 
			case 'B':
				setCursor(cursorCol_, cursorRow_ + first);
				return true;
			// CSI <n> C -- moves cursor n columns forward (right)
			case 'C':
				setCursor(cursorCol_ + first, cursorRow_);
				return true;
			// CSI <n> D -- moves cursor n columns back (left)
			case 'D': // cursor backward
				setCursor(cursorCol_ - first, cursorRow_);
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
						fillRect(helpers::Rect(cursorCol_, cursorRow_, cols_, cursorRow_ + 1), ' ');
						fillRect(helpers::Rect(0, cursorRow_ + 1, cols_, rows_), ' ');
						return true;
					case 1:
						updateCursorPosition();
						fillRect(helpers::Rect(0, 0, cols_, cursorRow_), ' ');
						fillRect(helpers::Rect(0, cursorRow_, cursorCol_ + 1, cursorRow_ + 1), ' ');
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
				unsigned l = std::min(cols_ - cursorCol_, first);
				fillRect(helpers::Rect(cursorCol_, cursorRow_, cursorCol_ + l, cursorRow_ + 1), ' ');
				first -= l;
				// while there is enough stuff left to be larger than a line, erase entire line
				l = cursorRow_ + 1;
				while (first >= cols_ && l < rows_) {
					fillRect(helpers::Rect(0, l, cols_, ++l), ' ');
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
			case ';': {
				int second = parseNumber(1);
				switch (pop()) {
					// CSI <row> ; <col> H -- sets cursor position
				    case 'H':
					case 'f':
						setCursor(second - 1, first - 1);
						return true;
				    default:
					    break;
				}
				break;
			}
		    // otherwise we are dealing with an unrecognized sequence, return false
			default:
				break;
		}
		return false;
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
			// cursor show/hide
			case 25:
				// TODO needs to be actually implemented
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
				return false;

			/* Enable/disable bracketed paste mode. When enabled, if user pastes code in the window, the contents should be enclosed with ESC [200~ and ESC[201~ so that the client app can determine it is contents of the clipboard (things like vi might otherwise want to interpret it. */
			case 2004:
				// TODO
				return true;

		default:
			return false;
		}
	}

	// ]0;C:\WINDOWS\SYSTEM32\wsl.exe BEL

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
				return true;
			/* Bold / bright foreground. */
			case 1:
				font_.setBold(true);
				return true;
			/* Underline */
			case 4:
				font_.setUnderline(true);
				return true;
			/* Disable underline. */
			case 24:
				font_.setUnderline(false);
				return true;
			/* 30 - 37 are dark foreground colors, handled in the default case. */
			/* Foreground default. */
			case 39:
				fg_ = defaultFg();
				return true;
			/* 40 - 47 are dark background color, handled in the default case. */
			/* Background default */
			case 49:
				bg_ = defaultBg();
				return true;

			/* 90 - 97 are bright foreground colors, handled in the default case. */
			/* 100 - 107 are bright background colors, handled in the default case. */

		    default:
				if (value >= 30 && value <= 37)
					fg_ = palette_[value - 30];
				else if (value >= 40 && value <= 47) 
					bg_ = palette_[value - 40];
				else if (value >= 90 && value <= 97) 
					fg_ = palette_[value - 82];
				else if (value >= 100 && value <= 107) 
					bg_ = palette_[value - 92];
				else 
					return false;
				return true;
		}
	}


	void VT100::fillRect(helpers::Rect const & rect, Char::UTF8 c, Color fg, Color bg, Font font) {
		for (unsigned row = rect.top; row < rect.bottom; ++row) {
			for (unsigned col = rect.left; col < rect.right; ++col) {
				Cell & cell = defaultLayer_->at(col, row);
				cell.fg = fg;
				cell.bg = bg;
				cell.font = font;
				cell.c = c;
			}
		}
	}

	void VT100::scrollDown(unsigned lines) {
		ASSERT(lines < rows_);
		for (unsigned r = 0, re = rows_ - lines; r < re; ++r) {
			for (unsigned c = 0; c < cols_; ++c) {
				defaultLayer_->at(c, r) = defaultLayer_->at(c, r + lines);
			}
		}
	}

} // namespace vterm
#include <algorithm>
#include <iostream>


#include "helpers/strings.h"

#include "vt100.h"

#define BEL '\x07'
#define ESC '\033'




namespace vterm {


	VT100::VT100(unsigned cols, unsigned rows) :
		IOTerminal(cols, rows),
		defaultFg_(Color::White),
		defaultBg_(Color::Black),
		fg_(Color::White),
		bg_(Color::Black),
		font_(),
		cursorCol_(0),
        cursorRow_(0) {
	}

	void VT100::processInputStream(char * buffer, size_t & size) {
		std::cout << "Received " << size << " bytes" << std::endl;
		buffer_ = buffer;
		bufferEnd_ = buffer_ + size;
		{

			// lock the terminal for updates while decoding
			std::lock_guard<std::mutex> g(m_);

			while (! eof()) {
				char c = pop();
				switch (c) {
				case ESC: {
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
				case '\n':
					++cursorRow_;
					cursorCol_ = 0;
					break;
				/* Carriage return sets cursor column to 0. 
				 */
				case '\r':
					cursorCol_ = 0;
					break;
				/* default variant is to print the character received to current cell.
				 */
				default: {
					// make sure that cursor is positioned within the terminal (previous advances may have placed it outside of the terminal area)
					updateCursorPosition();
					// get the cell and update its contents
					Cell & cell = defaultLayer_->at(cursorCol_, cursorRow_);
					cell.fg = fg_;
					cell.bg = bg_;
					cell.font = font_;
					cell.c = c;
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
		ASSERT(* (buffer_ - 1) == ESC);
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
				while (! condPop(BEL)) {
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
		ASSERT(*(buffer_-1) == ESC); 
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
				font_ = Font();
				fg_ = defaultFg_;
				bg_ = defaultBg_;
				return true;
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
		switch (id) {
			// cursor blinking
		    case 12:
			// cursor show/hide
			case 25:
				// TODO needs to be actually implemented
		default:
			return false;
		}
	}

	bool VT100::parseOSC() {
		return false;
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
		NOT_IMPLEMENTED;
	}

} // namespace vterm
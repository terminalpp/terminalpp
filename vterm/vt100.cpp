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

	}

#ifdef FOOBAR

	unsigned VT100::processBytes(char * buffer, unsigned size) {
		// if no terminal is attached to the connector, do nothing - these bytes are lost
		if (terminal() == nullptr)
			return size;
		// otherwise process the bytes by searching for escape sequences, writing the rest to the terminal's layer
		{
			char * end = buffer + size;
			VirtualTerminal::ScreenBuffer sb(terminal()->screenBuffer());
			while (buffer < end) {
				if (*buffer == 0x1b) {
					char * start = buffer;
					if (!parseEscapeSequence(sb, buffer, end))
						return size - (end - start);
				} else if (*buffer >= 0x20 && *buffer < 0x7f) {
					char * start = buffer;
					while (*buffer >= 0x20 && *buffer <= 0x7f)
						++buffer;
					print(sb, start, buffer - start);
				} else {
				    print(sb, buffer, 1);
					++buffer;
				}
			}
		}
		repaint(0, 0, terminal()->cols(), terminal()->rows());
		return size;
	}


	bool VT100::parseEscapeSequence(VirtualTerminal::ScreenBuffer & sb, char * & buffer, char * end) {
		ASSERT(* buffer == ESC);
		char * c = buffer++; // skip the escape char
		switch (*buffer) {
		case ']':
			if (parseSpecial(sb, ++buffer, end))
				return true;
			break;
		case '[':
			if (parseCSI(sb, ++buffer, end))
			    return true;
			break;
		default:
			break;
		}
		buffer = c; // restore
		if (!skipEscapeSequence(buffer, end)) {
			buffer = c;
			return false;
		} else {
			std::cout << "Invalid sequence: ESC";
			for (++c; c < buffer; ++c)
				std::cout << ' ' << *c;
			std::cout << std::endl;
		}
		return true;
	}

	VT100::Argument<int> VT100::parseNumber(char * & buffer, char * end, int defaultValue) {
		int value = 0;
		bool specified = false;
		while (buffer != end && helpers::IsDecimalDigit(*buffer)) {
			value = value * 10 + helpers::DecCharToNumber(*buffer++);
			specified = true;
		}
		if (!specified)
			value = defaultValue;
		return Argument<int>{value, specified};
	}

	bool VT100::parseSetter(VirtualTerminal::ScreenBuffer & sb, char * & buffer, char * end) {
		Argument<int> kind = parseNumber(buffer, end);
		if (!kind.specified)
			return false;
		bool value;
		if (buffer == end)
			return false;
		if (*buffer == 'h')
			value = true;
		else if (*buffer == 'l')
			value = false;
		else
			return false;

		switch (kind.value) {
		case 25:
			return true;
		}

		return false;

	}

	bool VT100::parseCSI(VirtualTerminal::ScreenBuffer & sb,char * & buffer, char * end) {
		if (buffer == end)
			return false;
		if (*buffer == '?')
			return parseSetter(sb, ++buffer, end);

		Argument<int> first = parseNumber(buffer, end, 1);
		switch (*buffer) {
		// CSI <n> A -- moves cursor n rows up 
		case 'A':
			++buffer;
			return true;
		// CSI <n> B -- moves cursor n rows down 
		case 'B':
			++buffer;
			return true;
		// CSI <n> C -- moves cursor n columns forward (right)
		case 'C':
			++buffer;
			setCursor(sb, cursorCol_ + first.value, cursorRow_);
			return true;
		// CSI <n> D -- moves cursor n columns back (left)
		case 'D': // cursor backward
			++buffer;
			return true;
		// CSI <n> X -- erases n characters to the right with space, w/o changing cursor pos
		case 'X':
			++buffer;
			return true;



		// extra argument
		case ';': { 
			Argument<int> second = parseNumber(++buffer, end, 1);
			switch (*buffer) {
			// CSI <row> ; <col> H - sets cursor position
			case 'H': 
				++buffer;
				setCursor(sb, second.value, first.value);
				return true;
			}
		}
		}

		return false;
	}

	bool VT100::parseSpecial(VirtualTerminal::ScreenBuffer & sb, char * & buffer, char * end) {
		return false;
	}




	void VT100::scrollLineUp(VirtualTerminal::ScreenBuffer & sb) {
		for (unsigned row = 0, re = sb.rows() - 1; row != re; ++row) {
			for (unsigned col = 0, ce = sb.cols(); col != ce; ++col) {
				sb.at(col, row) = sb.at(col, row + 1);
			}
		}
	}

	void VT100::updateCursorPosition(VirtualTerminal::ScreenBuffer & sb) {
		// current position should never be greater, only equal, but this is a bit of defensive programming...
		if (cursorCol_ >= sb.cols()) {
			cursorCol_ = 0;
			++cursorRow_;
		}
		if (cursorRow_ >= sb.rows()) {
			//cursorRow_ = 0;
			cursorRow_ = sb.rows() - 1;
			//scrollLineUp(sb);
		}
	}

	void VT100::print(VirtualTerminal::ScreenBuffer & sb, char const * text, unsigned length) {
		char const * end = text + length;
		while (text != end) {
			updateCursorPosition(sb);
			auto & c = sb.at(cursorCol_, cursorRow_);
			c.font = font_;
			c.fg = colorFg_;
			c.bg = colorBg_;
			c.c = Char::UTF8(*text);
			// move to next character
			++cursorCol_;
			if (*text == '\n') {
				++cursorRow_;
				cursorCol_ = 0;
			}
			++text;
		}
	}

	void VT100::clear(VirtualTerminal::ScreenBuffer & sb, unsigned cols) {
		while (cols > 0) {
			updateCursorPosition(sb);
			auto & c = sb.at(cursorCol_, cursorRow_);
			c.font = font_;
			c.fg = colorFg_;
			c.bg = colorBg_;
			c.c = ' ';
			// move to next character
			++cursorCol_;
		}
	}

	void VT100::setCursor(VirtualTerminal::ScreenBuffer & sb,unsigned col, unsigned row) {
		cursorCol_ = col;
		cursorRow_ = row;
		while (cursorCol_ >= sb.cols()) {
			cursorCol_ -= sb.cols();
			++cursorRow_;
		}
		if (cursorRow_ >= sb.rows())
			cursorRow_ = sb.rows() - 1;
	}





	/*

	void VT100::InitializeEscapeSequences() {
		if (root_ == nullptr) {
			root_ = new MatcherNode();

			AddEscapeCode(EscapeCode::CursorUp, { ESC, 'A' });
			AddEscapeCode(EscapeCode::CursorDown, { ESC, 'B' });
			AddEscapeCode(EscapeCode::CursorForward, { ESC, 'C' });
			AddEscapeCode(EscapeCode::CursorBackward, { ESC, 'D' });

			AddEscapeCode(EscapeCode::CursorUp, { CSI, NUMBER, 'A' });
			AddEscapeCode(EscapeCode::CursorDown, { CSI, NUMBER, 'B' });
			AddEscapeCode(EscapeCode::CursorForward, { CSI, NUMBER, 'C' });
			AddEscapeCode(EscapeCode::CursorBackward, { CSI, NUMBER, 'D' });

			AddEscapeCode(EscapeCode::EraseCharacter, { CSI, NUMBER, 'X' });
			AddEscapeCode(EscapeCode::EraseInDisplay, { CSI, NUMBER, 'J' });

			AddEscapeCode(EscapeCode::ShowCursor, { CSI, '?', NUMBER, 'h', CHECK_ARG(0,25) });
			AddEscapeCode(EscapeCode::HideCursor, { CSI, '?', NUMBER, 'l', CHECK_ARG(0,25) });
		}

	}

	VT100::Node * VT100::Node::createNode(VT100::EscapeCode code, VT100::EscapeSequence::Token const * t, VT100::EscapeSequence::Token const * end) {
		if (t == end)
			return new TerminalNode(code);
		switch (t->kind) {
		case EscapeSequence::Token::Kind::Character:
		case EscapeSequence::Token::Kind::Number:
			return new MatcherNode();
		case EscapeSequence::Token::Kind::DefaultIntegerArg:
			return new DefaultIntegerArgNode(t->index, t->value);
		case EscapeSequence::Token::Kind::CheckIntegerArg:
			return new CheckIntegerArgNode(t->index);
		default:
			UNREACHABLE;
		}
	}

	/*
	bool VT100::Node::match(char * buffer, char * end, EscapeSequence & seq) {
		// if the node is terminal node, return its code
		if (result_ != EscapeCode::Unknown) {
			seq.code = result_;
			seq.next = buffer;
			return true;
		}
		// if there is no more input, matching fails
		if (buffer == end)
			return false;
		// if the node is optional node, we parse a number first and then continue matching
		if (isNumber_) {
			if (!helpers::IsDecimalDigit(*buffer)) {
				//seq.addIntegerArgument(0, false);
			}
			else {
				int result = 0;
				while (buffer != end && helpers::IsDecimalDigit(*buffer))
					result = result * 10 + helpers::DecCharToNumber(*buffer++);
				//seq.addIntegerArgument(result, true);
			}
			// if there is no more input after the number, matching fails
			if (buffer == end)
				return false;
		}
		// finally, do the matching based on the character
		auto i = transitions_.find(*buffer++);
		if (i == transitions_.end())
			return false;
		return i->second->match(buffer, end, seq);
	}

	void VT100::Node::addMatch(EscapeCode code, char const * begin, char const * end) {
		if (begin == end) {
			ASSERT(result_ == EscapeCode::Unknown) << "Ambiguous match";
			result_ = code;
		}
		else {
			if (*begin == NUMBER) {
				isNumber_ = true;
				++begin;
			}
			ASSERT(begin != end) << "Number must not be last token";
			auto i = transitions_.find(*begin);
			if (i == transitions_.end())
				i = transitions_.insert(std::make_pair(*begin, new Node())).first;
			i->second->addMatch(code, begin + 1, end);
		}
	} */
	/*
	void VT100::AddEscapeCode(EscapeCode code, std::initializer_list<EscapeSequence::Token> match) {
//		ASSERT(*match.begin() == ESC) << "Escape sequences must start with escape character";
		root_->addMatch(code, match.begin() + 1, match.end());

	} */


#endif FOOBAR

} // namespace vterm
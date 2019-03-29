#include <iostream>

#include "helpers/strings.h"

#include "vt100.h"

#define BEL '\x07'
#define ESC '\033'




namespace vterm {


	VT100::VT100(unsigned cols, unsigned rows):
		IOTerminal(cols, rows),
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

			while (buffer_ < bufferEnd_) {
				switch (*buffer_) {
				case ESC: {
					// make a copy of buffer in case process escape sequence advances it and then fails
					char * b = buffer_;
					if (!processEscapeSequence()) {
						buffer_ = b;
						if (!skipEscapeSequence()) {
							size = b - buffer;
							return;
						}
						else {
							std::cout << "unknown escape sequence: ";
							for (++b; b < buffer_; ++b)
								std::cout << *b;
							std::cout << std::endl;
						}
					}
					break;
				}
						  /* New line simply moves to next line.
						   */
				case '\n':
					++cursorRow_;
					cursorCol_ = 0;
					++buffer_;
					break;
					/* default variant is to print the character received to current cell.
					 */
				default: {
					// make sure that cursor is positioned within the terminal (previous advances may have placed it outside of the terminal area)
					updateCursorPosition();
					// get the cell and update its contents
					Cell & c = defaultLayer_->at(cursorCol_, cursorRow_);
					c.fg = fg_;
					c.bg = bg_;
					c.font = font_;
					c.c = *buffer_++;
					// move to next column
					++cursorCol_;
				}
				}
			}
		}
		repaintAll();
	}


	bool VT100::skipEscapeSequence() {
		ASSERT(*buffer_ == ESC);
		++buffer_;
		if (buffer_ == bufferEnd_)
			return false;
		// if the next character is `[` then we match CSI, otherwise we just sklip it as the second byte of the sequence
		switch (*buffer_) {
			case '[': {
				++buffer_;
				// the CSI is now followed by arbitrary number parameter bytes (0x30 - 0x3f)
				while (true) {
					if (buffer_ == bufferEnd_)
						return false;
					if (*buffer_ < 0x30 || *buffer_ > 0x3f)
						break;
					++buffer_;
				}
				// then by any number of intermediate bytes (0x20 - 0x2f)
				while (true) {
					if (buffer_ == bufferEnd_)
						return false;
					if (*buffer_ < 0x20 || *buffer_ > 0x2f)
						break;
					++buffer_;
				}
				// and then by the final byte
				if (buffer_ == bufferEnd_)
					return false;
				++buffer_;
				break;
			}
			case ']': {
				// TODO for now we only handle BEL
				while (*buffer_ != BEL) {
					if (buffer_ == bufferEnd_)
						return false;
					++buffer_;
				}
				++buffer_;
				break;
			}
		}
		return true;
	}

	bool VT100::processEscapeSequence() {
		return false;
	}

	bool VT100::processCSI() {
		return false;
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
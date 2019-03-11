#include <iostream>

#include "helpers/strings.h"

#include "vt100.h"

#define ESC '\033'
#define CSI '\033','['
#define NUMBER -1

namespace vterm {

	VT100::Node * VT100::root_ = nullptr;


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
					EscapeSequence seq;
					// see if the sequence is matched
					if (root_->match(buffer + 1, end, seq)) {
						switch (seq.code) {
						case EscapeCode::EraseCharacter:
							//print(sb, "ECH", 3);
							break;
						case EscapeCode::CursorForward:
							//print(sb, "CUF", 3);
							break;
						}
						buffer = seq.next;
					// if the sequence is not matched, try matching it without understanding and report the unmatched sequence. 
					} else {
						char * seqEnd = skipUnknownEscapeSequence(buffer, end);
						// in case and end of buffer is detected before the end of the sequence, then we wait for the next bytes to be processed
						if (seqEnd == buffer)
							return size - (end - buffer);
						buffer = seqEnd;
					}
				} else {
					print(sb, buffer, 1);
					++buffer;
				}
			}
		}
		repaint(0, 0, terminal()->cols(), terminal()->rows());
		return size;
	}

	char * VT100::skipUnknownEscapeSequence(char * buffer, char * end) {
		char * seqEnd = buffer;
		ASSERT(*seqEnd == ESC);
		++seqEnd;
		if (seqEnd == end)
			return buffer;
		// if the next character is `[` then we match CSI, otherwise we just sklip it as the second byte of the sequence
		if (*(seqEnd++) == '[') {
			// the CSI is now followed by arbitrary number parameter bytes (0x30 - 0x3f)
			while (true) {
				if (seqEnd == end)
					return buffer;
				if (*seqEnd < 0x30 || *seqEnd > 0x3f)
					break;
				++seqEnd;
			}
			// then by any number of intermediate bytes (0x20 - 0x2f)
			while (true) {
				if (seqEnd == end)
					return buffer;
				if (*seqEnd < 0x20 || *seqEnd > 0x2f)
					break;
				++seqEnd;
			}
			// and then by the final byte
			if (seqEnd == end)
				return buffer;
			++seqEnd;
		}
		std::cout << "Invalid sequence: ESC";
		for (++buffer; buffer != seqEnd; ++buffer)
			std::cout << ' ' << *buffer;
		std::cout << std::endl;
		return seqEnd;
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







	void VT100::InitializeEscapeSequences() {
		if (root_ == nullptr) {
			root_ = new Node();

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
		}

	}

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
				seq.addIntegerArgument(0, false);
			}
			else {
				int result = 0;
				while (buffer != end && helpers::IsDecimalDigit(*buffer))
					result = result * 10 + helpers::DecCharToNumber(*buffer++);
				seq.addIntegerArgument(result, true);
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
	}

	void VT100::AddEscapeCode(EscapeCode code, std::initializer_list<char> match) {
		ASSERT(*match.begin() == ESC) << "Escape sequences must start with escape character";
		root_->addMatch(code, match.begin() + 1, match.end());

	}


} // namespace vterm
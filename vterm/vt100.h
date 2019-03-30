#pragma once

#include "terminal.h"

namespace vterm {

	/** IO terminal with VT100 escape sequences decoder. 

	    Support at least the following escape sequences :

        https://docs.microsoft.com/en-us/windows/console/console-virtual-terminal-sequences

	    There will most likely be some more to explore - check the ST terminal as well.
	*/
	class VT100 : public virtual IOTerminal {
	public:
		VT100(unsigned cols, unsigned rows);
		
	protected:

		char top() {
			return (buffer_ == bufferEnd_) ? 0 : *buffer_;
		}

		char pop() {
			if (buffer_ != bufferEnd_)
				return *buffer_++;
			else
				return 0;
		}

		bool condPop(char what) {
			if (top() == what) {
				pop();
				return true;
			} else {
				return false;
			}
		}

		bool eof() {
			return buffer_ == bufferEnd_;
		}

		void processInputStream(char * buffer, size_t & size) override;

		bool skipEscapeSequence();

		unsigned parseNumber(unsigned defaultValue = 0);

		bool parseEscapeSequence();

		/** Parses supported Control Sequence Introducers (CSIs), which start with ESC[.
		 */
		bool parseCSI();

		/** Parses a setter, which is ESC[? <id> (h|l). 
		 */
		bool parseSetter();

		/** Parses supported Operating System Commands (OSCs), which start with ESC].
		 */
		bool parseOSC();


		void updateCursorPosition() {
			if (cursorCol_ >= cols_) {
				cursorCol_ = 0;
				++cursorRow_;
			}
			if (cursorRow_ >= rows_) {
				scrollDown(cursorRow_ - rows_ + 1);
				cursorRow_ = rows_ - 1;
			}
		}


		/** Updates the cursor position. 
		 */
		void setCursor(unsigned col, unsigned row) {
			cursorCol_ = col;
			cursorRow_ = row;
		}

		/** Fills the given rectangle with character, colors and font.
		 */
		void fillRect(helpers::Rect const & rect, Char::UTF8 c, Color fg, Color bg, Font font = Font());
		void fillRect(helpers::Rect const & rect, Char::UTF8 c, Font font = Font()) {
			fillRect(rect, c, defaultFg_, defaultBg_, font);
		}

		void scrollDown(unsigned lines);



		Color defaultFg_;
		Color defaultBg_;

		Color fg_;
		Color bg_;
		Font font_;
		unsigned cursorCol_;
		unsigned cursorRow_;

		char * buffer_;
		char * bufferEnd_;

	};

} // namespace vterm
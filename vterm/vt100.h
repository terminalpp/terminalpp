#pragma once

#include "terminal.h"

namespace vterm {

	/** Palette of colors. 
	 */
	class Palette {
	public:
		Palette(size_t size) :
			size_(size),
			colors_(new Color[size]) {
		}

		Palette(std::initializer_list<Color> colors);

		Palette(Palette const & from);

		Palette(Palette && from);

		~Palette() {
			delete colors_;
		}

		void fillFrom(Palette const & from);

		size_t size() const {
			return size_;
		}

		Color const & operator [] (size_t index) const {
			return color(index);
		}

		Color & operator [] (size_t index) {
			return color(index);
		}

		Color const & color(size_t index) const {
			ASSERT(index < size_);
			return colors_[index];
		}

		Color & color(size_t index) {
			ASSERT(index < size_);
			return colors_[index];
		}

		static Palette const Colors16;

	private:
		size_t size_;
		Color * colors_;
	};

	/** IO terminal with VT100 escape sequences decoder. 

	    Support at least the following escape sequences :

        https://docs.microsoft.com/en-us/windows/console/console-virtual-terminal-sequences

	    There will most likely be some more to explore - check the ST terminal as well.
	*/
	class VT100 : public virtual IOTerminal {
	public:
		VT100(unsigned cols, unsigned rows, Palette const & palette, unsigned defaultFg, unsigned defaulrBg);

		Palette const & palette() const {
			return palette_;
		}
		
	protected:

		void doResize(unsigned cols, unsigned rows) override;

		Color const & defaultFg() const {
			return palette_[defaultFg_];;
		}

		Color const & defaultBg() const {
			return palette_[defaultBg_];;
		}

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

		/** Executes the VT100 SGR (Set Graphics Rendition) command. 
		 */
		bool SGR(unsigned value);

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
			fillRect(rect, c, defaultFg(), defaultBg(), font);
		}

		void scrollDown(unsigned lines);



		Palette palette_;
		unsigned defaultFg_;
		unsigned defaultBg_;

		Color fg_;
		Color bg_;
		Font font_;
		unsigned cursorCol_;
		unsigned cursorRow_;

		char * buffer_;
		char * bufferEnd_;

	};

} // namespace vterm
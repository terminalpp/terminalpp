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

	    There will most likely be some more to explore - check the ST terminal as well. Here is xterm:

		https://invisible-island.net/xterm/ctlseqs/ctlseqs.pdf


	*/
	class VT100 : public virtual IOTerminal {
	public:

		static constexpr char const * const SEQ = "VT100";
		static constexpr char const * const UNKNOWN_SEQ = "VT100_UNKNOWN";

		// events ---------------------------------------------------------------------------------------

		typedef helpers::EventPayload<std::string, helpers::Object> TitleEvent;

		/** Triggered when terminal client requests terminal title change. 
		 */
		helpers::Event<TitleEvent> onTitleChange;

		VT100(unsigned cols, unsigned rows, Palette const & palette, unsigned defaultFg, unsigned defaulrBg);

		Palette const & palette() const {
			return palette_;
		}

		// methods --------------------------------------------------------------------------------------

		void keyDown(Key k) override;

		/* VT100 is not interested in keyups, the key is sent on keydown event alone. 
		 */
		void keyUp(Key k) override { };

		void charInput(Char::UTF8 c) override;


		
	protected:

		/** Describes the keys understood by the VT100 and the sequences to be sent when the keys are emited. 
		 */
		class KeyMap {
		public:
			KeyMap();

			std::string const* getSequence(Key k);

		private:

			void addKey(Key k, char const * seq);

			void addVTModifiers(Key k, char const* seq1, char const* seq2);

			std::unordered_map<unsigned, std::string> & getModifierMap(Key k);


			std::vector<std::unordered_map<unsigned, std::string>> keys_;
		};


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

		bool parseCSI(unsigned first);

		bool parseCSI(unsigned first, unsigned second);

		bool parseCSI(unsigned first, unsigned second, unsigned third);

		bool parseCSI(unsigned first, unsigned second, unsigned third, unsigned fourth);

		/** Parses a setter, which is ESC[? <id> (h|l). 
		 */
		bool parseSetter();

		/** Parses supported Operating System Commands (OSCs), which start with ESC].
		 */
		bool parseOSC();

		/** Executes the VT100 SGR (Set Graphics Rendition) command. 
		 */
		bool SGR(unsigned value);

		/** Parses SGR 38 and 48 (extended color either RGB or from palette 
		 */
		bool parseExtendedColor(Color& storeInto);

		void updateCursorPosition() {
			if (cursorPos_.col >= cols_) {
				cursorPos_.col = 0;
				++cursorPos_.row;
			}
			if (cursorPos_.row >= rows_) {
				scrollDown(cursorPos_.row - rows_ + 1);
				cursorPos_.row = rows_ - 1;
			}
		}


		/** Updates the cursor position. 
		 */
		void setCursor(unsigned col, unsigned row);

		/** Fills the given rectangle with character, colors and font.
		 */
		void fillRect(helpers::Rect const & rect, Char::UTF8 c, Color fg, Color bg, Font font = Font());
		void fillRect(helpers::Rect const & rect, Char::UTF8 c, Font font = Font()) {
			fillRect(rect, c, fg_, bg_, font);
		}

		void scrollDown(unsigned lines);



		Palette palette_;
		unsigned defaultFg_;
		unsigned defaultBg_;

		Color fg_;
		Color bg_;
		Font font_;

		char * buffer_;
		char * bufferEnd_;

		static KeyMap keyMap_;

	};

} // namespace vterm
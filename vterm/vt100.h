#pragma once

#include <unordered_map>

#include "terminal.h"

namespace vterm {


	/** IO terminal with VT100 escape sequences decoder. 

	    Support at least the following escape sequences :

        https://docs.microsoft.com/en-us/windows/console/console-virtual-terminal-sequences

	    There will most likely be some more to explore - check the ST terminal as well. Here is xterm:

		https://invisible-island.net/xterm/ctlseqs/ctlseqs.pdf

        https://www.vt100.net/docs/vt102-ug/chapter5.html    

        https://chromium.googlesource.com/apps/libapps/+/a5fb83c190aa9d74f4a9bca233dac6be2664e9e9/hterm/doc/ControlSequences.md    


	*/

	class VT100 : public PTYTerminal {
	public:

		// log levels

		static constexpr char const* const SEQ = "VT100";
		static constexpr char const* const SEQ_UNKNOWN = "VT100_UNKNOWN";
		static constexpr char const* const SEQ_WONT_SUPPORT = "VT100_WONT_SUPPORT";


		static Palette ColorsXTerm256();

		VT100(unsigned cols, unsigned rows,PTY* pty, size_t bnufferSize = 51200);


		bool captureMouse() const override {
			return mouseMode_ != MouseMode::Off;
		}



		// Terminal key actions

		void keyDown(Key k) override;

		void keyUp(Key k) override;

		void keyChar(Char::UTF8 c) override;

		// terminal mouse actions

		void mouseDown(unsigned col, unsigned row, MouseButton button) override;

		void mouseUp(unsigned col, unsigned row, MouseButton button) override;

		void mouseWheel(unsigned col, unsigned row, int by) override;

		void mouseMove(unsigned col, unsigned row) override;

		// terminal clipboard actions

		void paste(std::string const& what) override;

	protected:

		/** TODO should this happen here, or when the buffer is actually flipped? 
		 */
		void doOnResize(unsigned cols, unsigned rows) override {
			// the actual screen has been resized already
			// reset scroll regions
			state_.resize(cols, rows);
			otherState_.resize(cols, rows);
			// reset the alternate buffer's cells
			otherScreen_.resize(cols, rows);
			// send the information to the attached process
			PTYTerminal::doOnResize(cols, rows);
		}

		size_t doProcessInput(char * buffer, size_t size) override;

	private:

		/** Returns true if valid escape sequence has been parsed from the buffer regardless of whether that sequence is supported or not. Return false if an incomplete possibly valid sequence has been found and therefore more data must be read from the terminal in order to proceed.
		 */
		bool parseEscapeSequence(char*& buffer, char* end);

		/** Operating System Commands end with either an ST (ESC \), or with BEL. For now, only setting the terminal window command is supported.

		    TODO make the parser nicer if more OSC sequences are necessary? 
		 */
		bool parseOSCSequence(char*& buffer, char* end);
		void processCSISequence();

		void processSetterOrGetter();
		void processSaveOrRestore();
		void processSGR();
		Color processSGRExtendedColor(size_t& i);

		/** Desrcibes parsed CSI sequence.

			The CSI sequence may have a first character and a last character which determine the kind of sequence and an arbitrary number of integer arguments.
		 */
		class CSISequence {
		public:

			enum class ParseResult {
				Valid,
				Invalid,
				Incomplete,
			}; // CSISequence::ParseResult
			 
			char firstByte() const {
				return firstByte_;
			}

			char finalByte() const {
				return finalByte_;
			}

			size_t numArgs() const {
				return args_.size();
			}

			/** Returns the index-th argument.

				If such argument was not parsed or explicitly set to default value, returns the default default value, which is 0.
			 */
			unsigned arg(size_t index) const {
				if (index >= args_.size())
					return 0;
				return args_[index].first;
			}

			unsigned operator [] (size_t index) const {
				if (index >= args_.size())
					return 0;
				return args_[index].first;
			}

			/** Sets the default value of given argument.

				If the argument was already specified by the user, setting the default value is ignored.
			 */
			void setArgDefault(size_t index, unsigned value) {
				while (args_.size() <= index)
					args_.push_back(std::make_pair(0, false));
				std::pair<unsigned, bool>& arg = args_[index];
				if (!arg.second)
					arg.first = value;
			}

			/** Creates an empty CSI sequence.
			 */
			CSISequence() :
				firstByte_(0),
				finalByte_(0),
				start_(nullptr),
				end_(nullptr) {
			}

			ParseResult parse(char *& start, char const* end);

			/** Parses the sequence from the given terminal's current position.

				Returns true if the sequence was parsed successfully (or raises the InvalidCSISequence eror *after* parsing the whole sequence if the sequence does have unsupported structure). Returns false if the buffer did not contain whole sequence.
			 */
			bool parse(VT100& term);

		private:

			static bool IsParameterByte(char c) {
				return (c >= 0x30) && (c <= 0x3f);
			}

			static bool IsIntermediateByte(char c) {
				return (c >= 0x20) && (c <= 0x2f);
			}

			static bool IsFinalByte(char c) {
				// no need to check the upper bound - char is signed
				return (c >= 0x40) /*&& (c <= 0x7f) */;
			}

			friend std::ostream& operator << (std::ostream& s, CSISequence const& seq) {
				s << std::string(seq.start_, seq.end_ - seq.start_);
				return s;
			}

			char firstByte_;
			char finalByte_;
			/** List of arguments.

				Each argument is specified by a tuple of the value and a boolean flag which determines if the value was explicitly given by the parsed sequence (true), or if it is default value (false).
			 */
			std::vector<std::pair<unsigned, bool>> args_;
			char const* start_;
			char const* end_;
		};


		/** The state of the terminal.

			The state is kept in a separate class so that it can be easily swapped or archived in a stack.
		 */
		class State {
		public:
			Color fg;
			Color bg;
			Font font;
			/** Start of the scrolling region (inclusive row).
			 */
			unsigned scrollStart = 0;

			/** End of the scrolling region (exclusive row).
			 */
			unsigned scrollEnd = 0;


			void resize(unsigned cols, unsigned rows) {
				MARK_AS_UNUSED(cols);
				scrollStart = 0;
				scrollEnd = rows;
			}

		};

		/** Describes the keys understood by the VT100 and the sequences to be sent when the keys are emited.
		 */
		class KeyMap {
		public:
			KeyMap();

			std::string const* getSequence(Key k);

		private:

			void addKey(Key k, char const* seq);

			void addVTModifiers(Key k, char const* seq1, char const* seq2);

			std::unordered_map<unsigned, std::string>& getModifierMap(Key k);

			std::vector<std::unordered_map<unsigned, std::string>> keys_;
		};

		static KeyMap KeyMap_;

		enum class MouseMode {
			Off,
			Normal,
			ButtonEvent,
			All
		};

		/** Determines encoding used to send mouse information.
		 */
		enum class MouseEncoding {
			Default,
			UTF8,
			SGR
		};

		/** Contains actual state of all known mouse buttons and keyboard modifiers. 
		 */
		class InputState {
		public:
			bool shift;
			bool ctrl;
			bool alt;
			bool win;
			bool mouseLeft;
			bool mouseRight;
			bool mouseWheel;

			InputState() :
				shift(false),
				ctrl(false),
				alt(false),
				win(false),
				mouseLeft(false),
				mouseRight(false),
				mouseWheel(false) {
			}

			void keyUpdate(Key k, bool value) {
				if (k | Key::Shift)
					shift = value;
				if (k | Key::Ctrl)
					ctrl = value;
				if (k | Key::Alt)
					alt = value;
				if (k | Key::Win)
					win = value;
			}

			void buttonUpdate(MouseButton button, bool value) {
				switch (button) {
					case MouseButton::Left:
						mouseLeft = value;
						break;
					case MouseButton::Right:
						mouseRight = value;
						break;
					case MouseButton::Wheel:
						mouseWheel = value;
						break;
					default:
						UNREACHABLE;
				}
			}

		};

		void updateCursorPosition() {
			ASSERT(screen_.cursor().col < screen_.cols());
			if (screen_.cursor().row == state_.scrollEnd) {
				deleteLine(1, state_.scrollStart);
				--screen_.cursor().row;
			}
			// if cursor row is not valid, just set it to the last row 
			if (screen_.cursor().row >= screen_.rows())
				screen_.cursor().row = screen_.rows() - 1;
		}

		void setMouseMode(MouseMode mode) {
			mouseMode_ = mode;
			// TODO event on mouse capture state change
		}

		
		
		unsigned encodeMouseButton(MouseButton btn);

		void sendMouseEvent(unsigned button, unsigned col, unsigned row, char end);








		/** Updates the cursor position.
		 */
		void setCursor(unsigned col, unsigned row);

		/** Fills the given rectangle with character, colors and font.
		 */
		void fillRect(helpers::Rect const& rect, Char::UTF8 c, Color fg, Color bg, Font font = Font());
		void fillRect(helpers::Rect const& rect, Char::UTF8 c, Font font = Font()) {
			fillRect(rect, c, state_.fg, state_.bg, font);
		}

		void deleteLine(unsigned lines, unsigned from);
		void insertLine(unsigned lines, unsigned from);

		void deleteCharacters(unsigned num);
		void insertCharacters(unsigned num);



		/** Current palette used by the terminal.

			The palette determines the 256 fixed colors used by the terminal.
		 */
		Palette palette_;

		/** Index into the palette corresponding to default foreground color.
		 */
		unsigned defaultFg_;

		/** Index to the palette corresponding to default background color.
		 */
		unsigned defaultBg_;


		State state_;
		InputState inputState_;

		// alternate mode support

		State otherState_;
		Screen otherScreen_;

		MouseMode mouseMode_;

		MouseEncoding mouseEncoding_;

		/** Last mouse button pressed - we must keep it around so that we know what to send when mouse move information is reuested. 
		 */
		unsigned mouseLastButton_;


		bool alternateBuffer_;

		bool bracketedPaste_;
		bool applicationCursorMode_;
		bool applicationKeypadMode_;




		CSISequence seq_;

	};

} // namespace vterm
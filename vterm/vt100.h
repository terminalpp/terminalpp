#pragma once

#include <unordered_map>

#include "terminal.h"

namespace vterm {

	/** Palette of colors. 

	    Although vterm fully supports the true color rendering, for compatibility and shorter escape codes, the 256 color palette as defined for xterm is supported via this class. 

		The separation of the palette and the terminal allows very simple theming in the future, should this feature be implemented. 
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
			ASSERT(index < size_) << index << " -- " <<  size_;
			return colors_[index];
		}

		static Palette Colors16();
        static Palette ColorsXTerm256();

	private:
		size_t size_;
		Color * colors_;
	};

	/** IO terminal with VT100 escape sequences decoder. 

	    Support at least the following escape sequences :

        https://docs.microsoft.com/en-us/windows/console/console-virtual-terminal-sequences

	    There will most likely be some more to explore - check the ST terminal as well. Here is xterm:

		https://invisible-island.net/xterm/ctlseqs/ctlseqs.pdf

        https://www.vt100.net/docs/vt102-ug/chapter5.html    

        https://chromium.googlesource.com/apps/libapps/+/a5fb83c190aa9d74f4a9bca233dac6be2664e9e9/hterm/doc/ControlSequences.md    


	*/
	class VT100 : public Terminal::PTYBackend {
	public:

		static constexpr char const * const SEQ = "VT100";
		static constexpr char const* const SEQ_UNKNOWN = "VT100_UNKNOWN";
		static constexpr char const* const SEQ_WONT_SUPPORT = "VT100_WONT_SUPPORT";

		// events ---------------------------------------------------------------------------------------

		typedef helpers::EventPayload<std::string, helpers::Object> TitleEvent;

		/** Triggered when terminal client requests terminal title change. 
		 */
		helpers::Event<TitleEvent> onTitleChange;

		VT100(PTY * pty, Palette const & palette, unsigned defaultFg, unsigned defaultBg);

		Palette const & palette() const {
			return palette_;
		}

		// methods --------------------------------------------------------------------------------------

        void keyDown(Key k) override;

		void keyUp(Key k) override;

		void keyChar(Char::UTF8 c) override;

		void mouseMove(unsigned col, unsigned row) override;
		void mouseDown(unsigned col, unsigned row, MouseButton button) override;
		void mouseUp(unsigned col, unsigned row, MouseButton button) override;
		void mouseWheel(unsigned col, unsigned row, int offset) override;

        void paste(std::string const & what) override;

		bool captureMouse() const override {
			return mouseMode_ != MouseMode::Off;
		}
		
	protected:


		enum class MouseMode {
			Off,
			Normal,
			ButtonEvent,
			All
		};

        class CSISequence;

        class InvalidCSISequence : public helpers::Exception {
        public:
            InvalidCSISequence(CSISequence const & seq);
            InvalidCSISequence(std::string const & msg, CSISequence const & seq);
        };

        /** Desrcibes parsed CSI sequence. 
            
            The CSI sequence may have a first character and a last character which determine the kind of sequence and an arbitrary number of integer arguments. 
         */
        class CSISequence {
        public:
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
                std::pair<unsigned, bool> & arg = args_[index];
                if (! arg.second)
                    arg.first = value;
            }

            /** Creates an empty CSI sequence. 
             */
            CSISequence():
                firstByte_(0),
                finalByte_(0),
                start_(nullptr),
                end_(nullptr) {
            }

            /** Parses the sequence from the given terminal's current position. 
              
                Returns true if the sequence was parsed successfully (or raises the InvalidCSISequence eror *after* parsing the whole sequence if the sequence does have unsupported structure). Returns false if the buffer did not contain whole sequence. 
             */
            bool parse(VT100 & term);

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

            friend std::ostream & operator << (std::ostream & s, CSISequence const & seq) {
                s << std::string(seq.start_, seq.end_ - seq.start_);
                return s;
            }

            char firstByte_;
            char finalByte_;
            /** List of arguments.
             
                Each argument is specified by a tuple of the value and a boolean flag which determines if the value was explicitly given by the parsed sequence (true), or if it is default value (false). 
             */
            std::vector<std::pair<unsigned, bool>> args_;
            char const * start_;
            char const * end_;
        };

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

		void resize(unsigned cols, unsigned rows) override;

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

		void setMouseMode(MouseMode mode) {
			mouseMode_ = mode;
			// TODO event on mouse capture state change
		}

		size_t dataReceived(char * buffer, size_t size) override;

		bool skipEscapeSequence();

		unsigned parseNumber(unsigned defaultValue = 0);

		bool parseEscapeSequence();

		/** Parses supported Control Sequence Introducers (CSIs), which start with ESC[.
		 */
		void parseCSI(CSISequence & seq);

        Color parseExtendedColor(CSISequence & seq, size_t & i);

		/** Executes the VT100 SGR (Set Graphics Rendition) command. 
		 */
        void parseSGR(CSISequence & seq);

		/** Parses a setter, which is ESC[? <id> (h|l). 
		 */
        void parseSetterOrGetter(CSISequence & seq);

		/** Parses the private mode save or restore command. 
		 */
		void parseSaveOrRestore(CSISequence & seq);

		/** Parses supported Operating System Commands (OSCs), which start with ESC].
		 */
		bool parseOSC();

		void updateCursorPosition() {
			ASSERT(cursor().col < cols());
			if (cursor().row == state_.scrollEnd) {
				deleteLine(1, state_.scrollStart);
				--cursor().row;
		    }
			// if cursor row is not valid, just set it to the last row 
			if (cursor().row >= rows())
				cursor().row = rows() - 1;
		}

		/** Updates the cursor position. 
		 */
		void setCursor(unsigned col, unsigned row);

		/** Fills the given rectangle with character, colors and font.
		 */
		void fillRect(helpers::Rect const & rect, Char::UTF8 c, Color fg, Color bg, Font font = Font());
		void fillRect(helpers::Rect const & rect, Char::UTF8 c, Font font = Font()) {
			fillRect(rect, c, state_.fg, state_.bg, font);
		}

        void deleteLine(unsigned lines, unsigned from);
        void insertLine(unsigned lines, unsigned from);

		void deleteCharacters(unsigned num);


        void storeCursorInfo();

        void loadCursorInfo();

		/** Flips the current and other buffers. 
		 */
		void flipBuffer();

        void resetCurrentBuffer();

		unsigned encodeMouseButton(MouseButton btn);

		void sendMouseEvent(unsigned button, unsigned col, unsigned row, char end);

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
            unsigned scrollStart;

            /** End of the scrolling region (exclusive row). 
             */
            unsigned scrollEnd;


            void resize(unsigned cols, unsigned rows) {
				MARK_AS_UNUSED(cols);
                scrollStart = 0;
                scrollEnd = rows;
            }

        };

        /** State of the current buffer. 
         */
        State state_;

        /** State of the other buffer (depending on which buffer is active this is either the normal, or the alternate buffer).
         */
        State otherState_;

		/** Backup of the values for the inactive buffer. 
		 */
        Terminal::Buffer otherBuffer_;

        /** Stack for cursor information. 
         */
        std::vector<Terminal::Cursor> cursorStack_;

        /** True if application keypad mode is enabled. 
         */
        bool applicationKeypadMode_;

        /** True if application cursor mode is enabled. 
         */
        bool applicationCursorMode_;

		/** Determines whether alternate, or normal buffer is used. 
		 */
		bool alternateBuffer_;

        /** If true, uses the bracketed paste mode. 
         */
        bool bracketedPaste_;

		/** When the terminal is repainted, should all cells be invalidated? 

		    Such as when alternate buffer is switched, etc.
		 */
		bool invalidateAll_;

		MouseMode mouseMode_;

		/** Determines encoding used to send mouse information. 
		 */
		enum class MouseEncoding {
			Default,
			UTF8,
			SGR
		};

		MouseEncoding mouseEncoding_;

		/** When reporting mouse motion, the last button value is used. 
		 */
		unsigned mouseLastButton_;

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

		};

		InputState inputState_;

        /** Helper pointers to the communications buffer (current index and buffer end).
         */
		char* buffer_;
		char* bufferEnd_;

        


		static KeyMap keyMap_;

	};

} // namespace vterm
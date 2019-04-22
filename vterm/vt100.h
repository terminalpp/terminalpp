#pragma once

#include <unordered_map>

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
            int arg(size_t index) const {
                if (index >= args_.size())
                    return 0;
                return args_[index].first;
            }

            int operator [] (size_t index) const {
                if (index >= args_.size())
                    return 0;
                return args_[index].first;
            }

            /** Returns the index-th argument if given either by the sequence or explicitly by default value. 
             */
            int argExcplicit(size_t index) const {
                if (index >= args_.size())
                    THROW(InvalidCSISequence(*this));
                return args_[index].first;
            }

            /** Returns the index-th argument if it was given in the parsed sequence. 
             
                I.e. fails if the argument was not given at all, or only his default value was specified. 
            */
            int argGiven(size_t index) const {
                if (index >= args_.size())
                    THROW(InvalidCSISequence(*this));
                std::pair<int, bool> arg = args_[index];
                if (! arg.second)
                    THROW(InvalidCSISequence(*this));
                return arg.first;
            }

            /** Sets the default value of given argument. 
             
                If the argument was already specified by the user, setting the default value is ignored. 
             */
            void setArgDefault(size_t index, int value) {
                while (args_.size() <= index)
                    args_.push_back(std::make_pair(0, false));
                std::pair<int, bool> & arg = args_[index];
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
                return (c >= 0x40) && (c <= 0x7f);
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
            std::vector<std::pair<int, bool>> args_;
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
		void parseCSI(CSISequence & seq);

        Color parseExtendedColor(CSISequence & seq, size_t & i);

		/** Executes the VT100 SGR (Set Graphics Rendition) command. 
		 */
        void parseSGR(CSISequence & seq);

		/** Parses a setter, which is ESC[? <id> (h|l). 
		 */
        void parseSetterOrGetter(CSISequence & seq);

		/** Parses supported Operating System Commands (OSCs), which start with ESC].
		 */
		bool parseOSC();

		void updateCursorPosition() {
			while (cursorPos_.col >= cols_) {
				cursorPos_.col -= cols_;
				++cursorPos_.row;
			}
			ASSERT(cursorPos_.col < cols_);
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
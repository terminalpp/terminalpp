#pragma once

#include <vector>

#include "terminal.h"

namespace vterm {


    /** Terminal understanding the ANSI escape sequences. 
     
        Should this be named differently? 
     */
    class VT100 : public Terminal {
    public:

        class Palette;

        // log levels

		static constexpr char const* const SEQ = "VT100";
		static constexpr char const* const SEQ_UNKNOWN = "VT100_UNKNOWN";
		static constexpr char const* const SEQ_WONT_SUPPORT = "VT100_WONT_SUPPORT";


        VT100(int x, int y, int width, int height, Palette const * palette, PTY * pty, unsigned fps = 60, size_t ptyBufferSize = 10240);


    protected:
        class CSISequence;

        class OSCSequence;

        void updateSize(int cols, int rows) override;

        void mouseDown(int col, int row, ui::MouseButton button, ui::Key modifiers) override;
        void mouseUp(int col, int row, ui::MouseButton button, ui::Key modifiers) override;
        void mouseWheel(int col, int row, int by, ui::Key modifiers) override;
        void mouseMove(int col, int row, ui::Key modifiers) override;
        void keyChar(helpers::Char c) override;
        void keyDown(ui::Key key) override;
        void keyUp(ui::Key key) override;

        size_t processInput(char * buffer, size_t bufferSize) override;

        /** Parses ANSI and similar escape sequences in the input. 
         
            CSI, OSC and few others are supported. 
         */
        bool parseEscapeSequence(char *& buffer, const char * bufferEnd);

        /** Processes given CSI sequence. 
         
            Special sequences, such as get/set and save/restore sequences are delegated to their own functions, others are processed directly in the method. 
         */
        void parseCSISequence(CSISequence & seq);
        
        /** Parses CSI getters and setters.
         
            These are sequences `?` as the first byte, followed by arbitrary numbers, ending with either `h` or `l`. 
         */
        void parseCSIGetterOrSetter(CSISequence & seq, bool value);

        /** Parses the CSI save and restore commands. 
         
            These are sequences staring with `?` and ending with `r` or `s`. 

            At the moment, save and restore commands are not supported. 
         */
        void parseCSISaveOrRestore(CSISequence & seq);

        /** Parses special graphic rendition commands. 
         
            These have final byte of `m`, preceded by numeric arguments.
         */
        void parseSGR(CSISequence & seq);

        /** Parses the SGR extended color specification, i.e. either TrueColor RGB values, or 256 palette specification.
         */
        ui::Color parseSGRExtendedColor(CSISequence & seq, size_t & i);

        /** Parses the operating system sequence. 
         */
        void parseOSCSequence(OSCSequence & seq);

        unsigned encodeMouseButton(ui::MouseButton btn, ui::Key modifiers);
        void sendMouseEvent(unsigned button, int col, int row, char end);

        /** Updates cursor position before modifying the cell it points to. 
         
            This is because the cursor position may temporarily be outside of the terminal size, such as when last character on the line is written and the cursor moves to next column, which is outside of the screen. This cannot be fixed when it happens because if the cursor is then moved by non cell-changing means, such as position change, por carriage return, no changes should be observed. 

            This function on the other hand is called before each observable change and makes sure that the cursor is positioned within the terminal. If the reposition requires changes, scrolls the terminal accordingly.
         */
        void updateCursorPosition();

		/** Updates the cursor position.
		 */
		void setCursor(int col, int row);

		/** Fills the given rectangle with character, colors and font.
		 */
		void fillRect(ui::Rect const& rect, ui::Cell const & cell);

		void deleteCharacters(unsigned num);
		void insertCharacters(unsigned num);


        // TODO add comments and determine if it actually does what it is supposed to do, i.e. wrt CR and LF
		void invalidateLastCharPosition() {
			state_.lastCharCol = -1;
		}

		void markLastCharPosition() {
			if (state_.lastCharCol >= 0 && state_.lastCharCol < buffer_.cols() &&
                state_.lastCharRow >= 0 && state_.lastCharRow < buffer_.rows()) 
				buffer_.at(state_.lastCharCol, state_.lastCharRow) += ui::Attributes::EndOfLine();
		}

		void setLastCharPosition() {
			state_.lastCharCol = buffer_.cursor().col;
			state_.lastCharRow = buffer_.cursor().row;
		}

        enum class MouseMode {
            Off,
            Normal,
            ButtonEvent,
            All
        }; // VT100::MouseMode

        enum class MouseEncoding {
            Default,
            UTF8,
            SGR
        }; // VT100::MouseEncoding

        enum class CursorMode {
            Normal,
            Application
        }; // VT100::CursorMode

        enum class KeypadMode {
            Normal, 
            Application
        }; // VT100::KeypadMode

        struct State {

            /* Start of the scrolling region (inclusive row) */
            int scrollStart;
            /* End of the scrolling rehion (exclusive row) */
            int scrollEnd;
            
            /* Cell containing space and current fg, bg, decorations, etc. settings */
            ui::Cell cell;

            /* Location of the last valid character printed so that if followed by return, it can be set as line terminating. */
            int lastCharCol;
            int lastCharRow;

            /* Stack of loaded & saved cursor positions. */
            std::vector<ui::Point> cursorStack;

            State(int cols, int rows, ui::Color fg, ui::Color bg):
                scrollStart{0},
                scrollEnd{rows},
                cell{},
                lastCharCol{-1},
                lastCharRow{0} {
                cell << ' ' << ui::Foreground(fg) << ui::Background(bg) << ui::DecorationColor(fg);
                MARK_AS_UNUSED(cols);
            }

            void resize(int cols, int rows) {
                MARK_AS_UNUSED(cols);
                scrollStart = 0;
                scrollEnd = rows;
            }

        }; // VT100::State

        State state_;

        MouseMode mouseMode_;
        MouseEncoding mouseEncoding_;
        unsigned mouseLastButton_;
        /* Mouse buttons state */
        unsigned mouseButtonsDown_;

        CursorMode cursorMode_;
        KeypadMode keypadMode_;

        /* Determines whether pasted text will be surrounded by ESC[200~ and ESC[201~ */
        bool bracketedPaste_;

        /* Alternate screen & state */
        bool alternateBufferMode_;
        Buffer alternateBuffer_;
        State alternateState_;

        /* The palette used for the terminal. */
        Palette const * palette_;        

        std::string const * GetSequenceForKey(ui::Key key) {
            auto i = KeyMap_.find(key);
            if (i == KeyMap_.end())
                return nullptr;
            else
                return &(i->second);
        }

        static std::unordered_map<ui::Key, std::string> KeyMap_;

    }; // vterm::VT100

    /** Palette
     */
    class VT100::Palette {
    public:

        static Palette Colors16();
        static Palette XTerm256(); 

        Palette(size_t size, size_t defaultFg = 15, size_t defaultBg = 0):
            size_(size),
            defaultFg_(defaultFg),
            defaultBg_(defaultBg),
            colors_(new ui::Color[size]) {
            ASSERT(defaultFg < size && defaultBg < size);
        }

        Palette(std::initializer_list<ui::Color> colors, size_t defaultFg = 15, size_t defaultBg = 0);

        Palette(Palette const & from);

        Palette(Palette && from);

        ~Palette() {
            delete colors_;
        }

        size_t size() const {
            return size_;
        }

        ui::Color defaultForeground() const {
            return colors_[defaultFg_];
        }

        ui::Color defaultBackground() const {
            return colors_[defaultBg_];
        }

        ui::Color operator [] (size_t index) const {
            ASSERT(index < size_);
            return colors_[index];
        } 

        ui::Color & operator [] (size_t index) {
            ASSERT(index < size_);
            return colors_[index];
        } 

        ui::Color at(size_t index) const {
            return (*this)[index];
        }
        ui::Color & at(size_t index) {
            return (*this)[index];
        }

    private:

        size_t size_;
        size_t defaultFg_;
        size_t defaultBg_;
        ui::Color * colors_;

    }; // VT100::Palette


    /** Desrcibes parsed CSI sequence.

        The CSI sequence may have a first character and a last character which determine the kind of sequence and an arbitrary number of integer arguments.
     */
    class VT100::CSISequence {
    public:

        CSISequence():
            firstByte_{0},
            finalByte_{0} {
        }

        bool isValid() const {
            return firstByte_ != INVALID;
        }

        bool isComplete() const {
            return firstByte_ != INCOMPLETE;
        }

        char firstByte() const {
            return firstByte_;
        }

        char finalByte() const {
            return finalByte_;
        }

        size_t numArgs() const {
            return args_.size();
        }

        int operator [] (size_t index) const {
            if (index >= args_.size())
                return 0; // the default value for argument if not given
            return args_[index].first;
        }

        CSISequence & setDefault(size_t index, int value) {
            while (args_.size() <= index)
                args_.push_back(std::make_pair(0, false));
            std::pair<int, bool> & arg = args_[index];
            // because we set default args after parsing, we only change default value if it was not supplied
            if (!arg.second)
               arg.first = value;
            return *this;
        }

        /** If the given argument has the specified value, it is replaced with the new value given. 

            Returns true if the replace occured, false otherwise. 
            */
        bool conditionalReplace(size_t index, int value, int newValue) {
            if (index >= args_.size())
                return false;
            if (args_[index].first != value)
                return false;
            args_[index].first = newValue;
            return true;
        }

        /** Parses the CSI sequence from given input. 

         */
        static CSISequence Parse(char * & buffer, char const * end);

    private:

        char firstByte_;
        char finalByte_;
        std::vector<std::pair<int, bool>> args_;

        static constexpr char INVALID = -1;
        static constexpr char INCOMPLETE = -2;
        static constexpr int DEFAULT_ARG_VALUE = 0;

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
            if (!seq.isValid()) {
                s << "Invalid CSI Sequence";
            } else if (!seq.isComplete()) {
                s << "Incomplete CSI Sequence";
            } else {
                s << "\x1b[";
                if (seq.firstByte_ != 0) 
                    s << seq.firstByte_;
                if (!seq.args_.empty()) {
                    for (size_t i = 0, e = seq.args_.size(); i != e; ++i) {
                        if (seq.args_[i].second)
                            s << seq.args_[i].first;
                        if (i != e - 1)
                            s << ';';
                    }
                }
                s << seq.finalByte();
            }
            return s;
        }

    }; // VT100::CSISequence

    class VT100::OSCSequence {
    public:

        OSCSequence():
            num_{INVALID} {
        }

        int num() const {
            return num_;
        }

        std::string const & value() const {
            return value_;
        }

        bool isValid() const {
            return num_ != INVALID;
        }

        bool isComplete() const {
            return num_ != INCOMPLETE;
        }

        /** Parses the OSC sequence from given input. 
         */
        static OSCSequence Parse(char * & buffer, char const * end);

    private:

        int num_;
        std::string value_;

        static constexpr int INVALID = -1;
        static constexpr int INCOMPLETE = -2;

        friend std::ostream & operator << (std::ostream & s, OSCSequence const & seq) {
            if (!seq.isValid()) 
                s << "Invalid CSI Sequence";
            else if (!seq.isComplete()) 
                s << "Incomplete CSI Sequence";
            else 
                s << "\x1b]" << seq.num() << ';' << seq.value();
            return s;
        }


    }; // VT100::OSCSequence

} // namespace vterm
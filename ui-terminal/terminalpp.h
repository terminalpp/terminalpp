#pragma once

#include <vector>

#include "helpers/filesystem.h"

#include "tpp-lib/sequence.h"
#include "terminal.h"

namespace ui {

    class TppNewFileEvent {
    public:
        tpp::Sequence::NewFileRequest request;
        tpp::Sequence::NewFileResponse response;
    };

    class TppTransferStatusEvent {
    public:
        tpp::Sequence::TransferStatusRequest request;
        tpp::Sequence::TransferStatusResponse response;
    };

    typedef tpp::Sequence::DataRequest TppDataEvent;
    typedef tpp::Sequence::OpenFileRequest TppOpenFileEvent;

/*
    class NewRemoteFile {
    public:
        std::string const & hostname;
        std::string const & filename;
        std::string const & remotePath;
        size_t const size;
        int fileId;
    protected:
        friend class TerminalPP;

        NewRemoteFile(std::string const & hostname, std::string const & filename, std::string const & remotePath, size_t size):
            hostname(hostname),
            filename(filename),
            remotePath(remotePath),
            size(size),
            fileId(-1) {
        }
    };

    typedef helpers::EventPayload<NewRemoteFile, Widget> NewRemoteFileEvent;

    // TODO Or maybe give the sequence? 
    class RemoteData {
    public:
        int const fileId;
        bool valid;
        char const * const data;
        size_t const size;
        size_t const offset;
    protected:
        friend class TerminalPP;
        RemoteData(int fileId, bool valid, char const * data, size_t size, size_t offset):
            fileId(fileId),
            valid(valid),
            data(data),
            size(size),
            offset(offset) {
        }
    };

    typedef helpers::EventPayload<RemoteData, Widget> RemoteDataEvent;

    class TransferStatus {
    public:
        int const fileId;
        size_t transferredBytes;
    protected:
        friend class TerminalPP;
        TransferStatus(int fileId):
            fileId{fileId},
            transferredBytes{0} {
        }
    };

    typedef helpers::EventPayload<TransferStatus, Widget> TransferStatusEvent;

    class OpenRemoteFile {
    public:
        int const fileId;

    protected:
        friend class TerminalPP;

        OpenRemoteFile(int fileId):
            fileId(fileId) {
        }
    };

    typedef helpers::EventPayload<OpenRemoteFile, Widget> OpenRemoteFileEvent;

    */


    /** Terminal understanding the ANSI escape sequences. 
     
        Should this be named differently? 
     */
    class TerminalPP : public Terminal {
    public:

        class Palette;

        // log levels

        static helpers::Log SEQ;
        static helpers::Log SEQ_UNKNOWN;
        static helpers::Log SEQ_ERROR;
        static helpers::Log SEQ_WONT_SUPPORT;

        TerminalPP(int width, int height, Palette const * palette, PTY * pty, unsigned fps, size_t ptyBufferSize = 10240); // 1Mb buffer size

        Event<TppNewFileEvent> onTppNewFile;
        Event<TppDataEvent> onTppData;
        Event<TppTransferStatusEvent> onTppTransferStatus;
        Event<TppOpenFileEvent> onTppOpenFile;

        /** Determines whether the bold font is rendered in bright colors or not. 
         
            This settings only affects text that is selected bold, and its color is then set to one of the predefined colors (0-7). If enabled, the bright color indices (8-15) will be used instead.  
         */
        bool boldIsBright() const {
            return boldIsBright_;
        }

        void setBoldIsBright(bool value = true) {
            boldIsBright_ = value;
        }

    protected:
        class CSISequence;

        class OSCSequence;

        Color defaultForeground() const override;
        Color defaultBackground() const override;

        void updateSize(int width, int height) override;

        void mouseDown(int col, int row, MouseButton button, Key modifiers) override;
        void mouseUp(int col, int row, MouseButton button, Key modifiers) override;
        void mouseWheel(int col, int row, int by, Key modifiers) override;
        void mouseMove(int col, int row, Key modifiers) override;
        void keyChar(helpers::Char c) override;
        void keyDown(Key key) override;
        void keyUp(Key key) override;
        void paste(std::string const & contents) override;

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
        Color parseSGRExtendedColor(CSISequence & seq, size_t & i);

        /** Parses the operating system sequence. 
         */
        void parseOSCSequence(OSCSequence & seq);

        /** Parses the t++ specific control sequences. 

            See the extra documentation for more details.  
         */
        void parseTppSequence(tpp::Sequence && seq);

        /** Parses font size specifiers (double width, double height DEC modes) (ESC # x)
         */
        void parseFontSizeSpecifier(char kind);

        unsigned encodeMouseButton(MouseButton btn, Key modifiers);
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
		void fillRect(Rect const& rect, Cell const & cell);

		void deleteCharacters(unsigned num);
		void insertCharacters(unsigned num);

        /** Deletes lines and triggers the onLineScrolledOut event if appropriate. 
         
            The event is triggered only if the terminal is in normal mode and if the scroll region starts at the top of the window. 
         */
        void deleteLines(int lines, int top, int bottom, Cell const & fill);

        // TODO add comments and determine if it actually does what it is supposed to do, i.e. wrt CR and LF
		void invalidateLastCharPosition() {
			state_.lastCharCol = -1;
		}

		void markLastCharPosition() {
			if (state_.lastCharCol >= 0 && state_.lastCharCol < buffer_.cols() &&
                state_.lastCharRow >= 0 && state_.lastCharRow < buffer_.rows()) 
				buffer_.at(state_.lastCharCol, state_.lastCharRow) += Attributes::EndOfLine();
		}

		void setLastCharPosition() {
			state_.lastCharCol = buffer_.cursor().pos.x;
			state_.lastCharRow = buffer_.cursor().pos.y;
		}

        enum class MouseMode {
            Off,
            Normal,
            ButtonEvent,
            All
        }; // TerminalPP::MouseMode

        enum class MouseEncoding {
            Default,
            UTF8,
            SGR
        }; // TerminalPP::MouseEncoding

        enum class CursorMode {
            Normal,
            Application
        }; // TerminalPP::CursorMode

        enum class KeypadMode {
            Normal, 
            Application
        }; // TerminalPP::KeypadMode

        struct State {

            /* Start of the scrolling region (inclusive row) */
            int scrollStart;
            /* End of the scrolling rehion (exclusive row) */
            int scrollEnd;
            
            /* Cell containing space and current fg, bg, decorations, etc. settings */
            Cell cell;

            /* Location of the last valid character printed so that if followed by return, it can be set as line terminating. */
            int lastCharCol;
            int lastCharRow;

            /* Determines if we are currently at double height font top line.

               The second line is determined by the actual font used. 
             */
            bool doubleHeightTopLine;

            /* Stack of loaded & saved cursor positions. */
            std::vector<Point> cursorStack;

            State(int cols, int rows, Color fg, Color bg):
                scrollStart{0},
                scrollEnd{rows},
                cell{},
                lastCharCol{-1},
                lastCharRow{0},
                doubleHeightTopLine(false) {
                cell << ' ' << Foreground(fg) << Background(bg) << DecorationColor(fg);
                MARK_AS_UNUSED(cols);
            }

            void resize(int cols, int rows) {
                MARK_AS_UNUSED(cols);
                scrollStart = 0;
                scrollEnd = rows;
            }

        }; // TerminalPP::State

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

        /* Sequences & rendering options. */
        bool boldIsBright_;

        static std::string const * GetSequenceForKey(Key key) {
            auto i = KeyMap_.find(key);
            if (i == KeyMap_.end())
                return nullptr;
            else
                return &(i->second);
        }

        static std::unordered_map<Key, std::string> KeyMap_;

    }; // ui::terminalpp

    /** Palette
     */
    class TerminalPP::Palette {
    public:

        static Palette Colors16();
        static Palette XTerm256(); 

        Palette(size_t size, size_t defaultFg = 15, size_t defaultBg = 0):
            size_(size),
            defaultFg_(defaultFg),
            defaultBg_(defaultBg),
            colors_(new Color[size]) {
            ASSERT(defaultFg < size && defaultBg < size);
        }

        Palette(std::initializer_list<Color> colors, size_t defaultFg = 15, size_t defaultBg = 0);

        Palette(Palette const & from);

        Palette(Palette && from);

        ~Palette() {
            delete [] colors_;
        }

        size_t size() const {
            return size_;
        }

        Color defaultForeground() const {
            return colors_[defaultFg_];
        }

        Color defaultBackground() const {
            return colors_[defaultBg_];
        }

        void setDefaultForegroundIndex(size_t value) {
            defaultFg_ = value;
        }

        void setDefaultBackgroundIndex(size_t value) {
            defaultBg_ = value;
        }

        void setColor(size_t index, Color color) {
            ASSERT(index < size_);
            colors_[index] = color;
        }

        Color operator [] (size_t index) const {
            ASSERT(index < size_);
            return colors_[index];
        } 

        Color & operator [] (size_t index) {
            ASSERT(index < size_);
            return colors_[index];
        } 

        Color at(size_t index) const {
            return (*this)[index];
        }
        Color & at(size_t index) {
            return (*this)[index];
        }

    private:

        size_t size_;
        size_t defaultFg_;
        size_t defaultBg_;
        Color * colors_;

    }; // TerminalPP::Palette


    PROPERTY_BUILDER(BoldIsBright, bool, setBoldIsBright, TerminalPP);

    /** Desrcibes parsed CSI sequence.

        The CSI sequence may have a first character and a last character which determine the kind of sequence and an arbitrary number of integer arguments.
     */
    class TerminalPP::CSISequence {
    public:

        CSISequence():
            firstByte_{0},
            finalByte_{0} {
        }

        bool valid() const {
            return firstByte_ != INVALID;
        }

        bool complete() const {
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
            if (!seq.valid()) {
                s << "Invalid CSI Sequence";
            } else if (!seq.complete()) {
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

    }; // TerminalPP::CSISequence

    class TerminalPP::OSCSequence {
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

        bool valid() const {
            return num_ != INVALID;
        }

        bool complete() const {
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
            if (!seq.valid()) 
                s << "Invalid OSC Sequence";
            else if (!seq.complete()) 
                s << "Incomplete OSC Sequence";
            else 
                s << "\x1b]" << seq.num() << ';' << seq.value();
            return s;
        }


    }; // TerminalPP::OSCSequence

} // namespace vterm
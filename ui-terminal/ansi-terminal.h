#pragma once

#include <thread>

#include "helpers/log.h"
#include "helpers/process.h"
#include "helpers/locks.h"

#include "ui2/renderer.h"
#include "ui2/widget.h"
#include "ui2/traits/scrollable.h"

#include "pty.h"

namespace ui2 {

    class AnsiTerminal : public Widget, public PTY::Client, public Scrollable<AnsiTerminal> {
    public:

        class Palette;

        // ========================================================================================

        /** 
         */
        class Buffer : public ui2::Buffer {
        public:
            Buffer(int width, int height):
                ui2::Buffer{width, height} {
                fill();
            }

            void fill() {
                for (int x = 0; x < width(); ++x)
                    for (int y = 0; y < height(); ++y) {
                        at(x, y).setCodepoint('0' + (x + y) % 10).setBg(Color{static_cast<unsigned char>(x), static_cast<unsigned char>(y), static_cast<unsigned char>(x + y)});
                    }
            }

        protected:

            friend class AnsiTerminal;

            /** Fills the specified row with given character. 

                Copies larger and larger number of cells at once to be more efficient than simple linear copy. 
             */
            void fillRow(Cell * row, Cell const & fill, unsigned cols);



        }; // ui::AnsiTerminal::Buffer

        // ========================================================================================

        /**\name Log Levels.
         */
        //@{

        static helpers::Log SEQ;
        static helpers::Log SEQ_UNKNOWN;
        static helpers::Log SEQ_ERROR;
        static helpers::Log SEQ_WONT_SUPPORT;

        //@}

        AnsiTerminal(Palette * palette, int width = 0, int height = 0, int x = 0, int y = 0); 

        ~AnsiTerminal() override;

    protected:
        
        class CSISequence;
        class OSCSequence;

        enum class MouseMode {
            Off,
            Normal,
            ButtonEvent,
            All
        }; // AnsiTerminal::MouseMode

        enum class MouseEncoding {
            Default,
            UTF8,
            SGR
        }; // AnsiTerminal::MouseEncoding

        enum class CursorMode {
            Normal,
            Application
        }; // AnsiTerminal::CursorMode

        enum class KeypadMode {
            Normal, 
            Application
        }; // AnsiTerminal::KeypadMode

        class State {
        public:
            State(int cols, int rows):
                buffer{cols, rows},
                cursorMode{CursorMode::Normal},
                cursorVisible{true},
                cursorBlink{true},
                cursor{0,0},
                lastCharacter{0,0},
                mouseMode{MouseMode::Off},
                mouseEncoding{MouseEncoding::Default},
                scrollStart{0},
                scrollEnd{rows},
                inverseMode{false},
                lineDrawingSet{false},
                keypadMode{KeypadMode::Normal},
                bracketedPaste{false},
                maxHistoryRows{1000} {
            }

            void resize(int cols, int rows) {
                buffer.resize(cols, rows);
                buffer.fill();
            }

            void setCursor(int col, int row) {
                cursor = Point{col, row};
                // invalidate the last character's position
                lastCharacter = Point{-1, -1};
            }

            void saveCursor() {
                cursorStack_.push_back(cursor);
            }

            void restoreCursor() {
                if (cursorStack_.empty())
                    return;
                cursor = cursorStack_.back();
                cursorStack_.pop_back();
                if (cursor.x() >= buffer.width())
                    cursor.setX(buffer.width() - 1);
                if (cursor.y() >= buffer.height())
                    cursor.setX(buffer.height() - 1);
            }

            int historyRows() const {
                return history_.size();
            }

            void addHistoryRows(int numCols, Color defaultBg);

            void drawHistoryRows(Canvas & canvas, int start, int end);

            Buffer buffer;
            Cell cell;

            CursorMode cursorMode;
            bool cursorVisible;
            bool cursorBlink;
            /** Current cursor position. */
            Point cursor;
            /** Determines the coordinates of last valid cursor position so that newline cells can be identified. */
            Point lastCharacter;


            MouseMode mouseMode;
            MouseEncoding mouseEncoding;


            /** Start of the scrolling region (inclusive row) */
            int scrollStart;
            /** End of the scrolling rehion (exclusive row) */
            int scrollEnd;
            /** Determines whether inverse mode is active or not. */
            bool inverseMode;
            /** Determines whether the line drawing character set is currently active. */
            bool lineDrawingSet;

            KeypadMode keypadMode;
            /* Determines whether pasted text will be surrounded by ESC[200~ and ESC[201~ */
            bool bracketedPaste;

            size_t maxHistoryRows;

        protected:

            std::vector<Point> cursorStack_;

            std::deque<std::pair<int, Cell*>> history_;

        }; // AnsiTerminal::State

        /** \name Rendering
         */
        //@{

        void paint(Canvas & canvas) override;

        void setRect(Rect const & value) override;

        /** The contents canvas is as long as the terminal itself and any history rows. 
         */
        using Scrollable::getContentsCanvas;

        //@}

        /** \name User Input
         */
        //@{
        void mouseWheel(Event<MouseWheelEvent>::Payload & event) override;


        void keyChar(Event<Char>::Payload & event) override;
        void keyDown(Event<Key>::Payload & event) override;
        //@}



        /** \name PTY Interface 
         */
        //@{

        /** Parses the given input. Returns the number of bytes actually parsed. 
         */
        size_t processInput(char const * buffer, char const * bufferEnd) override;

        void ptyTerminated(ExitCode exitCode) override {
            Renderer::SendEvent([this, exitCode](){ });
        }
        //@}

        /** \name Input Parsing
         
            Input processing can happen in non-UI thread.
         */
        //@{

        void parseCodepoint(char32_t codepoint);

        void parseNotification();

        void parseTab();

        void parseLF();

        void parseCR();

        void parseBackspace();

        /** Parses ANSI and similar escape sequences in the input. 
         
            CSI, OSC and few others are supported. 
         */
        size_t parseEscapeSequence(char const * buffer, char const * bufferEnd);

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



        //@}

        /** \name Buffer manipulation. 
         */
        //@{
        /** Fills the given rectangle with character, colors and font.
         */
        void fillRect(Rect const& rect, Cell const & cell);

        void deleteCharacters(unsigned num);
        void insertCharacters(unsigned num);

        /** Deletes lines and triggers the onLineScrolledOut event if appropriate. 
         
            The event is triggered only if the terminal is in normal mode and if the scroll region starts at the top of the window. 
            */
        void deleteLines(int lines, int top, int bottom, Cell const & fill);

        /** Inserts given number of lines at given top row.
            
            Scrolls down all lines between top and bottom accordingly. Fills the new lines with the provided cell.
         */
        void insertLines(int lines, int top, int bottom, Cell const & fill);

        //@}

        void updateCursorPosition();

        helpers::PriorityLock bufferLock_;

        Palette * palette_;

        /** The terminal state. */
        State state_;
        /** Determines whether alternate mode is active or not. */
        bool alternateMode_;

        /** Backup of the normal state when alternate mode is enabled. */
        //State stateBackup_;

        /** If true, bold font means bright colors too. */
        bool boldIsBright_;

        static std::string const * GetSequenceForKey_(Key key) {
            auto i = KeyMap_.find(key);
            if (i == KeyMap_.end())
                return nullptr;
            else
                return &(i->second);
        }

        static std::unordered_map<Key, std::string> KeyMap_;
        static char32_t LineDrawingChars_[15];

    }; // ui2::AnsiTerminal

    // ============================================================================================

    /** Palette
     */
    class AnsiTerminal::Palette {
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

    }; // AnsiTerminal::Palette

    // ============================================================================================

    class AnsiTerminal::CSISequence {
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
        static CSISequence Parse(char const * & buffer, char const * end);

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

    }; // ui::AnsiTerminal::CSISequence

    // ============================================================================================

    class AnsiTerminal::OSCSequence {
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
        static OSCSequence Parse(char const * & buffer, char const * end);

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

    }; // ui::AnsiTerminal::OSCSequence

} // namespace ui
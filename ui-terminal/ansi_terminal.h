#pragma once

#include <thread>

#include "helpers/log.h"
#include "helpers/process.h"
#include "helpers/locks.h"

#include "ui/renderer.h"
#include "ui/widget.h"
#include "ui/traits/scrollable.h"
#include "ui/traits/selection_owner.h"

#include "pty.h"

namespace ui {

    class AnsiTerminal : public PublicWidget, public PTY::Client, public Scrollable<AnsiTerminal>, public SelectionOwner<AnsiTerminal> {
    public:

        class Palette;

        // ========================================================================================

        /** 
         */
        class Buffer : public ui::Buffer {
        public:
            Buffer(int width, int height):
                ui::Buffer{width, height},
                maxHistoryRows_{1000} {
            }

            void fill(Cell const & cell) {
                for (int y = 0; y < height(); ++y)
                    fillRow(rows_[y], cell, width());
            }

            /** Marks the given cell as end of line. 
             */
            void markAsLineEnd(Point p) {
                if (p.x() >= 0)
                    SetUnusedBits(at(p), END_OF_LINE);
            }

            /** Draws the buffer on given canvas. 
             
                The canvas must be the size of the buffer plus the size of the available history. 
             */
            void drawOnCanvas(Canvas & canvas, Color defaultBackground) const; 

            int historyRows() const {
                return static_cast<int>(history_.size());
            }

            void deleteLines(int lines, int top, int bottom, Cell const & fill, Color defaultBg);

            /** Inserts given number of lines at given top row.
                
                Scrolls down all lines between top and bottom accordingly. Fills the new lines with the provided cell.
            */
            void insertLines(int lines, int top, int bottom, Cell const & fill);    

            Point resize(int width, int height, bool resizeContents, Cell const & fill, Point cursor);

            /** Returns the contents of the selection from the buffer. 
             */
            std::string getSelectionContents(Selection const & selection) const;

        protected:

            friend class AnsiTerminal;

            /** Fills the specified row with given character. 

                Copies larger and larger number of cells at once to be more efficient than simple linear copy. 
             */
            void fillRow(Cell * row, Cell const & fill, unsigned cols);

            /** Adds the given row of cells as a history row. 

                If the row is longer than current width, it will be split and multiple actual rows will be added. If the history buffer is full, the appropriate ammount of oldest history rows will be deleted.   
             */
            void addHistoryRow(int cols, Cell * row);

            void resizeHistory(std::deque<std::pair<int, Cell*>> & oldHistory);

            /** The buffer history. */
            std::deque<std::pair<int, Cell*>> history_;

            bool isEndOfLine(Cell const & cell) const {
                return GetUnusedBits(cell) & END_OF_LINE;
            }

            int maxHistoryRows_;

            /** Flag designating the end of line in the buffer. 
             */
            static constexpr char32_t END_OF_LINE = 0x200000;

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

        /** \name Repainting
         
            To improve speed and reduce power consumption the terminal offers either immediate repaints (fps == 0), or allows the repaints to be buffered and only issued after a time (fps > 0). 
         */
        //@{
        /** Returns the maximum terminal refreshes per second. 
         */
        size_t fps() const {
            return fps_;
        }

        /** Sets the maximum frames per second for the terminal. 
         */
        virtual void setFps(unsigned value);

        /** Repaints the terminal. 
         
            If fps is set to 0 the terminal will request its repaint immediately, otherwise the repaint request is buffered and only one repaint request per the fps period is requested by the terminal. 
         */
        void repaint() override {
            if (fps_ == 0)
                Widget::repaint();
            else
                repaint_ = true;
        }
        //@}

        /** Returns true if the currently running application has requested mouse events to be sent to it. 
         */
        bool mouseCaptured() const {
            return (mouseMode_ != MouseMode::Off);
        }

        /** \name Events
         */
        //@{

        using Widget::onMouseDown;
        using Widget::onMouseUp;
        using Widget::onMouseWheel;
        using Widget::onMouseMove;

        using Widget::onKeyDown;
        using Widget::onKeyUp;
        using Widget::onKeyChar;
        
        /** Triggered when the title of the terminal changes. 
         */
        Event<std::string> onTitleChange;

        /** Triggered when the terminal receives a notification. 
         
            This means receiving the BEL character for now, but other notification forms might be possible in the future.
         */
        Event<void> onNotification; 

        /** Triggered when the attached application wishes to set the local clipboard contents. 

            Having this implemented as an event allows the containing application to deal with the event according to own security rules. 
         */
        Event<std::string> onSetClipboard;

        /** Triggered when the PTY attached to the temrinal terminates. 
         
            Has the exit code reported by the PTY as a payload. 
         */
        Event<helpers::ExitCode> onPTYTerminated;
        //@}

        void paste(Event<std::string>::Payload & e) override;

        using SelectionOwner::selection;
        using SelectionOwner::endSelectionUpdate;
        using SelectionOwner::updatingSelection;

        void startSelectionUpdate(Point from) {
            SelectionOwner::startSelectionUpdate(from + scrollOffset());
        }

        void clearSelection() override {
            Widget::clearSelection();
            SelectionOwner::clearSelection();
        }

        /** Returns the contents of the selection. 
         
            Trims the right of each line.
         */
        std::string getSelectionContents() const override;

        using Scrollable::scrollBy;



    protected:

        friend class SelectionOwner<AnsiTerminal>;
        
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
                cursor{0,0},
                lastCharacter{0,0},
                scrollStart{0},
                scrollEnd{rows},
                inverseMode{false} {
            }

            void reset(Color fg, Color bg);

            void resize(int cols, int rows, bool resizeContents, Color defaultBg) {
                Cell fill;
                fill.setBg(defaultBg);
                cursor = buffer.resize(cols, rows, resizeContents, fill, cursor);
                scrollStart = 0;
                scrollEnd = rows;
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


            Buffer buffer;
            Cell cell;

            /** Current cursor position. */
            Point cursor;
            /** Determines the coordinates of last valid cursor position so that newline cells can be identified. */
            Point lastCharacter;

            /** Start of the scrolling region (inclusive row) */
            int scrollStart;
            /** End of the scrolling rehion (exclusive row) */
            int scrollEnd;
            /** Determines whether inverse mode is active or not. */
            bool inverseMode;

        protected:

            std::vector<Point> cursorStack_;

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
        /** Reset the mouse buttons state on mouse leave. 
         */
        void mouseOut(Event<void>::Payload & event) override {
            mouseButtonsDown_ = 0;
            mouseLastButton_ = 0;
            Widget::mouseOut(event);
            
        }
        void mouseMove(Event<MouseMoveEvent>::Payload & event) override;
        void mouseDown(Event<MouseButtonEvent>::Payload & event) override;
        void mouseUp(Event<MouseButtonEvent>::Payload & event) override;
        void mouseWheel(Event<MouseWheelEvent>::Payload & event) override;

        unsigned encodeMouseButton(MouseButton btn, Key modifiers);
        void sendMouseEvent(unsigned button, Point coords, char end);

        /** Keyboard focus in triggers repaint as the cursor must be updated. 
         */
        void focusIn(Event<void>::Payload & event) override {
            Widget::focusIn(event);
            repaint();
        }

        /** Keyboard focus out triggers repaint as the cursor must be updated. 
         */
        void focusOut(Event<void>::Payload & event) override {
            Widget::focusOut(event);
            repaint();
        }

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
            sendEvent([this, exitCode](){
                Event<ExitCode>::Payload p{exitCode};
                // disable the terminal
                setEnabled(false);
                // call the event
                onPTYTerminated(p, this);
            });
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


        //@}

        void updateCursorPosition();

        /** Maximal refresh rate for the terminal. */
        std::atomic<unsigned> fps_;
        /** Repaint trigger. */
        std::atomic<bool> repaint_;
        /** Repaint trigger thread. */
        std::thread repainter_;

        helpers::PriorityLock bufferLock_;

        Palette * palette_;


        CursorMode cursorMode_;
        /** The cursor to be displayed. */
        Cursor cursor_;

        KeypadMode keypadMode_;

        MouseMode mouseMode_;
        MouseEncoding mouseEncoding_;
        /** Number of pressed mouse buttons to determine mouse capture. */
        unsigned mouseButtonsDown_;
        /** Last pressed mouse button for mouse move reporting. */
        unsigned mouseLastButton_;

        /** Determines whether the line drawing character set is currently active. */
        bool lineDrawingSet_;

        /* Determines whether pasted text will be surrounded by ESC[200~ and ESC[201~ */
        bool bracketedPaste_;

        /** The terminal state. */
        State state_;
        /** Backup of the normal state when alternate mode is enabled. */
        State stateBackup_;
        /** Determines whether alternate mode is active or not. */
        bool alternateMode_;

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

    }; // ui::AnsiTerminal

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
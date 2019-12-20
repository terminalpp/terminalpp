#pragma once

#include <type_traits>
#include <thread>
#include <deque>
#include <vector>

#include "helpers/filesystem.h"
#include "helpers/process.h"

#include "ui/canvas.h"
#include "ui/widget.h"
#include "ui/selection.h"
#include "ui/scrollable.h"
#include "ui/builders.h"

#include "tpp-lib/sequence.h"

/** \page vterm Terminal Widget
 
    \brief Widget capable of displaying terminal contents via attached PTY. 

    \section vtermHistory Terminal History
*/

namespace ui {

    /** The input buffer of the terminal and its length. 
     
        TODO this looks very generic and perhaps can exist in some generic place too.
     */
    struct InputProcessedEvent {
        char const * buffer;
        size_t size;
    };

    struct InputErrorEvent : public InputProcessedEvent {
        std::string error;

        InputErrorEvent(char const * buffer, size_t size, std::string const &error):
            InputProcessedEvent{buffer, size},
            error(error) {
        }
    };

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



    /** Terminal understanding the ANSI escape sequences. 
     
        Should this be named differently? 
     */
    class Terminal : public Widget, 
                     public Scrollable<Terminal>, 
                     public AutoScroller<Terminal>,
                     public SelectionOwner<Terminal> {
    public:

        class PTY;

        class Palette;

        /** The terminal buffer holds information about the cells and cursor. 

        */
        class Buffer {
        public:

            typedef helpers::SmartRAIIPtr<Buffer> Ptr;

            Buffer(int cols, int rows, Cell const & fill);

            Buffer(Buffer const & from);

            Buffer & operator = (Buffer const & other);

            ~Buffer();

            int cols() const {
                return cols_;
            }

            int rows() const {
                return rows_;
            }

            Cell const & at(int x, int y) const {
                ASSERT(x >= 0 && x <= cols_ && y >= 0 && y < rows_);
                return cells_[y][x];
            }

            Cell & at(int x, int y) {
                ASSERT(x >= 0 && x <= cols_ && y >= 0 && y < rows_);
                return cells_[y][x];
            }

            Cell const & at(Point pos) const {
                return at(pos.x, pos.y);
            }

            Cell & at(Point pos) {
                return at(pos.x, pos.y);
            }

            Cursor const & cursor() const {
                return cursor_;
            }

            Cursor & cursor() {
                return cursor_;
            }

            void lock() {
                lock_.lock();
            }

            void priorityLock() {
                lock_.priorityLock();
            }

            void unlock() {
                lock_.unlock();
            }

            /** Resizes the buffer. 
             
                The terminal to which the buffer belongs can be provided as an optional argument, in which case any lines that would be deleted after the resize would be added to the history of the specified terminal. 
             */
            void resize(int cols, int rows, Cell const & fill, Terminal * terminal);

            /** Inserts given number of lines at given top row.
                
                Scrolls down all lines between top and bottom accordingly. Fills the new lines with the provided cell.
                */
            void insertLines(int lines, int top, int bottom, Cell const & fill);

            /** Deletes given number of lines at given top row.
                
                Scrolls up all lines between top and bottom accordingly. Fills the new lines at the bottom with the provided cell.
                */
            void deleteLines(int lines, int top, int bottom, Cell const & fill);

            /** Returns the given line as a string.
             
                TODO This should preserve the attributes somehow.
             */
            std::string getLine(int line);

        private:

            /** Fills the specified row with given character. 

                Copies larger and larger number of cells at once to be more efficient than simple linear copy. 
                */
            void fillRow(Cell* row, Cell const& fill, unsigned cols);

            /** Resizes the cell buffer. 

                First creates the new buffer and clears it. Then we determine the latest line to be copied (since the client app is supposed to rewrite the current line). We also calculate the offset of this line to the current cursor line, which is important if the last line spans multiple terminal lines since we must adjust the cursor position accordingly then. 

                The line is the copied. 
            */
            void resizeCells(int newCols, int newRows, Cell const & fill, Terminal * terminal);


            /* Size of the buffer and the array of the cells. */
            int cols_;
            int rows_;
            Cell ** cells_;

            Cursor cursor_;

            /* Priority lock on the buffer. */
            helpers::PriorityLock lock_;

        }; // Terminal::Buffer 

        using Widget::setFocused;
        using Widget::setFocusStop;
        using Widget::setFocusIndex;
        using Widget::setEnabled;

        // log levels

        static helpers::Log SEQ;
        static helpers::Log SEQ_UNKNOWN;
        static helpers::Log SEQ_ERROR;
        static helpers::Log SEQ_WONT_SUPPORT;

        Terminal(int width, int height, Palette const * palette, PTY * pty, unsigned fps, size_t ptyBufferSize = 10240); // 1Mb buffer size

        /** Deletes the terminal and the attached PTY. 
         
            Waits for the reading and process exit terminating threads to finalize. 
         */
        ~Terminal() override;

        /** Triggered when the attached pseudoterminal terminates. 
         
            Note that when the terminal is deleted, its PTY is forcibly terminated at first, so the event will fire precisely once in the terminal lifetime. 
         */
        Event<helpers::ExitCode> onPTYTerminated;

        Event<std::string> onTitleChange;

        Event<void> onNotification;

        /** Triggered when a terminal line has been scrolled out. 
         
            It is assumed that the topmost line is always the line scrolled out. 
         */
        Event<void> onLineScrolledOut;

        /** Triggered when new input has been processed by the terminal. 
         */
        Event<InputProcessedEvent> onInput;
        Event<InputErrorEvent> onInputError;

        Event<TppNewFileEvent> onTppNewFile;
        Event<TppDataEvent> onTppData;
        Event<TppTransferStatusEvent> onTppTransferStatus;
        Event<TppOpenFileEvent> onTppOpenFile;

        PTY * pty() {
            return pty_;
        }

        /** Returns the title of the terminal. 
         */
        std::string const & title() const {
            return title_;
        }

        /** Returns the limit of lines that are retained outside of the terminal window. 
         */
        size_t historySizeLimit() const {
            return historySizeLimit_;
        }

        /** Sets the maximum number of lines past the terminal window the terminal is allowed to remember. 
         
            If set to 0, no scrollback is allowed.
         */
        void setHistorySizeLimit(size_t value) {
            if (value != historySizeLimit_) {
                historySizeLimit_ = value;
                // remove excess history data
                while (history_.size() > historySizeLimit_)
                    popHistoryLine();
                // resize window properly
                updateClientRect();
            }
        }

        /** Makes sure the terminal is scrolled so that the prompt is visible. 
         
            For the terminal, it simply scrolls past the history. 
         */
        void scrollToPrompt() {
            if (scrollable_ && history_.size() > 0)
                setScrollOffset(Point{0, static_cast<int>(history_.size())});
        }

        /** Determines whether the bold font is rendered in bright colors or not. 
         
            This settings only affects text that is selected bold, and its color is then set to one of the predefined colors (0-7). If enabled, the bright color indices (8-15) will be used instead.  
         */
        bool boldIsBright() const {
            return boldIsBright_;
        }

        void setBoldIsBright(bool value = true) {
            boldIsBright_ = value;
        }

        /** Sets the cursor for the terminal overwriting any settings currently stored. 
         */
        void setCursor(Cursor const & value) {
            Buffer::Ptr buffer = this->buffer(/* priority */false);
            buffer->cursor().activeCodepoint = value.activeCodepoint;
            buffer->cursor().activeColor = value.activeColor;
            buffer->cursor().activeBlink = value.activeBlink;
            buffer->cursor().inactiveCodepoint = value.inactiveCodepoint;
            buffer->cursor().inactiveColor = value.inactiveColor;
            buffer->cursor().inactiveBlink = value.inactiveBlink;
            // alternate screen buffer
            alternateBuffer_.cursor().activeCodepoint = value.activeCodepoint;
            alternateBuffer_.cursor().activeColor = value.activeColor;
            alternateBuffer_.cursor().activeBlink = value.activeBlink;
            alternateBuffer_.cursor().inactiveCodepoint = value.inactiveCodepoint;
            alternateBuffer_.cursor().inactiveColor = value.inactiveColor;
            alternateBuffer_.cursor().inactiveBlink = value.inactiveBlink;
        }

        void repaint() override {
            repaint_ = true;
        }

    protected:
        class CSISequence;

        class OSCSequence;

        using Scrollable<Terminal>::getChildrenCanvas;

        Color defaultForeground() const;
        Color defaultBackground() const;

        /** Returns the locked buffer. 
         */
        Buffer::Ptr buffer(bool priority = false) {
            if (priority)
                buffer_.priorityLock();
            else
                buffer_.lock();
            return Buffer::Ptr(buffer_, false);
        }

        /** Copies the contents of the terminal's buffer to the ui canvas. 
         */
        void paint(Canvas & canvas) override;

        /** Updates the FPS ratio of the terminal. 
         
            Note that the FPS is not precise since triggering the repaint event itself is not accounted for in the wait. This should not be a problem since the terminal is not really hard real-time affair.
         */
        void setFPS(unsigned value) {
            ASSERT(value != 0);
            fps_ = value;
        }

        void setTitle(std::string const & value) {
            if (title_ != value) {
                title_ = value;
                updateTitle(value);
            }
        }

        void send(char const * buffer, size_t size);

        void send(std::string const & str) {
            send(str.c_str(), str.size());
        }

        void updateSize(int width, int height) override;

        void updateFocused(bool value) override {
            Widget::updateFocused(value);
            repaint();
        }

        void mouseDown(int col, int row, MouseButton button, Key modifiers) override;
        void mouseUp(int col, int row, MouseButton button, Key modifiers) override;
        void mouseWheel(int col, int row, int by, Key modifiers) override;
        void mouseMove(int col, int row, Key modifiers) override;

        void mouseOut() override {
            if (scrollBarActive_) {
                scrollBarActive_ = false;
                repaint();
            }
            cancelSelection();
        }

        void keyChar(helpers::Char c) override;
        void keyDown(Key key) override;
        void keyUp(Key key) override;
        void paste(std::string const & contents) override;

        /** Returns the contents of the selection. 
         
            Trims the right of each line.
         */
        std::string getSelectionContents() const override;

        void selectionInvalidated() override {
            SelectionOwner<Terminal>::selectionInvalidated();
        }

        /** Called when the attached PTY has terminated. 
         */
        virtual void ptyTerminated(helpers::ExitCode exitCode) {
            onPTYTerminated(this, exitCode);
        }

        virtual void updateTitle(std::string const & title) {
            onTitleChange(this, title);
        }

        virtual void notify() {
            onNotification(this);
        }

        /** Appends the selected top lines to the terminal history. 
         */
        virtual void lineScrolledOut(Cell * line, int cols);

        /** Updates the client rectangle based on the history size and terminal height. 
         */
        void updateClientRect() {
            setClientSize(Point{width(), buffer_.rows() + static_cast<int>(history_.size())}); 
        }

        /** Enables or disables showing the terminal history. 
         */
        void enableScrolling(bool value = true) {
            if (scrollable_ != value) {
                scrollable_ = value;
            }
        }

        /** Removes from the terminal history the oldest line.
         */
        void popHistoryLine();

        /** Adds new line to the terminal history. 
         
            The method assumes the buffer lock to be valid and that its argument is a pointer to the array of cells of the line, i.e. that its width is the buffer's number of cells. It then trims all empty cells on the right and copies the rest to the line.
         */
        void addHistoryLine(Cell const * line, int cols);

        void resizeHistory(int newCols);

        /** Called by the terminal when new input data is available from the pty. 
         
            The method should obtain locked buffer and update it according to the received data. 
         */
        virtual size_t processInput(char * buffer, size_t bufferSize);

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
                MarkCellAsEndOfLine(buffer_.at(state_.lastCharCol, state_.lastCharRow));
		}

		void setLastCharPosition() {
			state_.lastCharCol = buffer_.cursor().pos.x;
			state_.lastCharRow = buffer_.cursor().pos.y;
		}

        static void MarkCellAsEndOfLine(Cell & cell, bool value = true) {
            cell.setReserved1(value);
        }

        static bool IsEndOfLine(Cell const & cell) {
            return cell.reserved1();
        }

        enum class MouseMode {
            Off,
            Normal,
            ButtonEvent,
            All
        }; // Terminal::MouseMode

        enum class MouseEncoding {
            Default,
            UTF8,
            SGR
        }; // Terminal::MouseEncoding

        enum class CursorMode {
            Normal,
            Application
        }; // Terminal::CursorMode

        enum class KeypadMode {
            Normal, 
            Application
        }; // Terminal::KeypadMode

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
                cell.setCodepoint(' ').setFg(fg).setDecoration(fg).setBg(bg);
                MARK_AS_UNUSED(cols);
            }

            void resize(int cols, int rows) {
                MARK_AS_UNUSED(cols);
                scrollStart = 0;
                scrollEnd = rows;
            }

        }; // Terminal::State

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

    private:

        /* Cells and cursor. */
        Buffer buffer_;

        /* Pseudoterminal for communication */
        PTY * pty_;

        /* Thread which reads data from the attached pseudoterminal. */
        std::thread ptyReader_;

        /* Thread which waits for the attached pty to be terminated. */
        std::thread ptyListener_;

        /* The requested number of redraws */
        volatile unsigned fps_;
        std::thread repainter_;
        std::atomic<bool> repaint_;

        /* Terminal title */
        std::string title_;

        /** Determines whethet the terminal is scrollable, or not. 
         */
        bool scrollable_;

        /** Determines if the history scrollbar is active or not (mouse over it)
         */
        bool scrollBarActive_;

        size_t historySizeLimit_;

        /* Historic lines, for each line we remember its size and the actual cells.
         */
        std::deque<std::pair<int, Cell *>> history_;

    }; // ui::terminalpp

    /** The pseudoterminal connection interface. 

        A pseudoterminal connection provides the simplest possible interface to the target process. 
     */
    class Terminal::PTY {
    public:
        /** Sends the given buffer to the target process. 
         */
        virtual void send(char const * buffer, size_t bufferSize) = 0;

        /** Receives up to bufferSize bytes which are stored in the provided buffer.
         
            Returns the number of bytes received. If there are no data available, blocks until the data is ready and then returns them. 

            If the attached process is terminated, should return immediately with 0 bytes read. 
         */
        virtual size_t receive(char * buffer, size_t bufferSize) = 0;

        /** Terminates the target process and returns immediately. 
         
            If the target process has already terminated, simply returns. 
         */
        virtual void terminate() = 0;

        /** Waits for the attached process to terminate and returns its exit code. 
         */
        virtual helpers::ExitCode waitFor() = 0;

        virtual void resize(int cols, int rows) = 0;

		/** Virtual destructor. 
		 */
		virtual ~PTY() {
		}

    }; // ui::Terminal::PTY

    inline void Terminal::send(char const * buffer, size_t size) {
        pty_->send(buffer, size);
        // TODO check errors
    }



    template<>
    inline void Canvas::copyBuffer<Terminal::Buffer>(int x, int y, Terminal::Buffer const & buffer) {
        int xe = std::min(x + buffer.cols(), width()) - x;
        int ye = std::min(y + buffer.rows(), height()) - y;
        for (int by = 0; by < ye; ++by)
            for (int bx = 0; bx < xe; ++bx)
                if (Cell * c = at(Point(x + bx, y + by))) 
                    *c = buffer.at(bx, by);
    }




    PROPERTY_BUILDER(HistorySizeLimit, size_t, setHistorySizeLimit, Terminal);


    /** Palette
     */
    class Terminal::Palette {
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

    }; // Terminal::Palette


    PROPERTY_BUILDER(BoldIsBright, bool, setBoldIsBright, Terminal);

    /** Desrcibes parsed CSI sequence.

        The CSI sequence may have a first character and a last character which determine the kind of sequence and an arbitrary number of integer arguments.
     */
    class Terminal::CSISequence {
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

    }; // Terminal::CSISequence

    class Terminal::OSCSequence {
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


    }; // Terminal::OSCSequence

} // namespace vterm
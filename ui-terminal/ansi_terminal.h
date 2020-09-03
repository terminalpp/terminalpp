#pragma once

#include <unordered_map>
#include <unordered_set>

#include "ui/canvas.h"

#include "ui/mixins/selection_owner.h"

#include "tpp-lib/pty.h"
#include "tpp-lib/pty_buffer.h"

#include "csi_sequence.h"
#include "osc_sequence.h"

namespace ui {

    /** The terminal alone. 
     
        The simplest interface to the rerminal, no history, selection, etc?
     */
    class AnsiTerminal : public virtual Widget, public tpp::PTYBuffer<tpp::PTYMaster> {
    public:
        using Cell = Canvas::Cell;
        class Buffer;
        class State;
        class Palette;

    /**\name Log Levels.
     */
    //@{
    public:
        static Log SEQ;
        static Log SEQ_UNKNOWN;
        static Log SEQ_ERROR;
        static Log SEQ_WONT_SUPPORT;
        static Log SEQ_SENT;
    //@}

    public:
        AnsiTerminal(tpp::PTYMaster * pty, Palette * palette);

        ~AnsiTerminal() override;

        void resize(Size const & size) override {
            if (rect().size() == size)
                return;
            bool scrollToTerminal;
            // under lock, update history and buffers
            {
                std::lock_guard<PriorityLock> g{bufferLock_.priorityLock(), std::adopt_lock};
                scrollToTerminal = scrollOffset().y() == historyRows();    
                resizeHistory(size.width());
                resizeBuffers(size);
                pty_->resize(size.width(), size.height());
                // TODO update size? 

            }
            Widget::resize(size);
            if (scrollToTerminal)
                setScrollOffset(Point{0, historyRows()});
        }

    /** \name Events
     */

    public:
        VoidEvent onNotification;
        StringEvent onTitleChange;
        StringEvent onClipboardSetRequest;

    /** \name Widget 
     */
    //@{

    protected:

        Size contentsSize() const override {
            if (alternateMode_)
                return Widget::contentsSize();
            else 
                return Size{width(), height() + historyRows()};
        }

        void paint(Canvas & canvas) override;

    //@}

    /** \name User Input
     */
    //@{

    protected:

        void keyDown(KeyEvent::Payload & e) override ;

        void keyUp(KeyEvent::Payload & e) override;

        void keyChar(KeyCharEvent::Payload & e) override;

        void mouseMove(MouseMoveEvent::Payload & e) override;

        void mouseDown(MouseButtonEvent::Payload & e) override;

        void mouseUp(MouseButtonEvent::Payload & e) override;

        void mouseWheel(MouseWheelEvent::Payload & e) override;

        unsigned encodeMouseButton(MouseButton btn, Key modifiers);

        void sendMouseEvent(unsigned button, Point coords, char end);

    private:

        /** Number of pressed mouse buttons to determine mouse capture. */
        unsigned mouseButtonsDown_ = 0;
        /** Last pressed mouse button for mouse move reporting. */
        unsigned mouseLastButton_ = 0;

        static std::unordered_map<Key, std::string> KeyMap_;
        static std::unordered_set<Key> PrintableKeys_;

    //}


    /** \name Terminal State and scrollback buffer
     */
    //@{
    
    public:
        int historyRows() const {
            return static_cast<int>(historyRows_.size());
        }

        int maxHistoryRows() const {
            return maxHistoryRows_;
        }

        void setMaxHistoryRows(int value) {
            if (value != maxHistoryRows_) {
                maxHistoryRows_ = std::max(value, 0);
                while (historyRows_.size() > static_cast<size_t>(maxHistoryRows_)) {
                    delete [] historyRows_.front().second;
                    historyRows_.pop_front();
                }
            }
        }

    protected:

        /** Returns current cursor position. 
         */
        Point cursorPosition() const;

        void setCursorPosition(Point position);

        /** Inserts given number of lines at given top row.
            
            Scrolls down all lines between top and bottom accordingly. Fills the new lines with the provided cell.
        */
        void insertLines(int lines, int top, int bottom, Cell const & fill);    

        /** Deletes lines and triggers the onLineScrolledOut event if appropriate. 
         
            The event is triggered only if the terminal is in normal mode and if the scroll region starts at the top of the window. 
            */
        void deleteLines(int lines, int top, int bottom, Cell const & fill);

        void addHistoryRow(Cell * row, int cols);

    protected:

        void resizeHistory(int width);
        void resizeBuffers(Size size);


        // TODO change to int
        void deleteCharacters(unsigned num);
        void insertCharacters(unsigned num);


        void updateCursorPosition();

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

        Palette * palette_;

        CursorMode cursorMode_ = CursorMode::Normal;
        /** The actual cursor, which can be updated by the terminal client. */
        Canvas::Cursor cursor_;
        /** The default cursor as specified by the configuration. */
        Canvas::Cursor defaultCursor_;
        /** Color of the inactive cursor rectangle. */
        Color inactiveCursorColor_ = Color::Green;

        KeypadMode keypadMode_ = KeypadMode::Normal;

        MouseMode mouseMode_ = MouseMode::Off;
        MouseEncoding mouseEncoding_ = MouseEncoding::Default;

        /** Determines whether the line drawing character set is currently active. */
        bool lineDrawingSet_ = false;

        /* Determines whether pasted text will be surrounded by ESC[200~ and ESC[201~ */
        bool bracketedPaste_ = false;

        /** If true, bold font means bright colors too. */
        bool boldIsBright_ = false;

        /** Determines whether alternate mode is active or not. */
        bool alternateMode_ = false;

        /** Current state and its backup. The states are swapped and current state kind is determined by the alternateMode(). 
         */
        State * state_;
        State * stateBackup_;
        PriorityLock bufferLock_;

        int maxHistoryRows_;
        std::deque<std::pair<int, Cell*>> historyRows_;


    //@}

    /** \name Input Processing
     */
    //@{
    protected:
        size_t received(char * buffer, char const * bufferEnd) override;

        void parseCodepoint(char32_t cp);
        void parseNotification();
        void parseTab();
        void parseLF();
        void parseCR();
        void parseBackspace();
        size_t parseEscapeSequence(char const * buffer, char const * bufferEnd);

        size_t parseTppSequence(char const * buffer, char const * bufferEnd);

        /** Called when `t++` sequence is parsed & received by the terminal. 
         
            The default implementation simply calls the event handler, if registered. 
         */
        /*
        virtual void processTppSequence(TppSequenceEvent seq) {
            UIEvent<TppSequenceEvent>::Payload p{seq};
            onTppSequence(p, this);
        } */

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

        static char32_t LineDrawingChars_[15];


    //@}


    }; // ui::AnsiTerminal

    // ============================================================================================

    /** Terminal's own buffer. 
     
        Like canvas buffer, but has support for tagging characters that are end of line and provides scrolling history. 
     */
    class AnsiTerminal::Buffer : public ui::Canvas::Buffer {
    public:
        Buffer(Size const & size):
            ui::Canvas::Buffer(size) {
        }

        void insertLine(int top, int bottom, Cell const & fill);

        std::pair<Cell *, int> copyRow(int row, Color defaultBg);

        void deleteLine(int top, int bottom, Cell const & fill);

        void markAsLineEnd(Point p) {
            if (p.x() >= 0)
                SetUnusedBits(at(p), END_OF_LINE);
        }

        static bool IsLineEnd(Cell & c) {
            return GetUnusedBits(c) & END_OF_LINE;
        }

        Point cursorPosition() const {
            return cursorPosition_;
        }

        void setCursorPosition(Point pos) {
            cursorPosition_ = pos;
        }

        void setCursor(Canvas::Cursor const & value, Point position) {
            cursor_ = value;
            cursorPosition_ = position;
        }

        void resize(Size size, Cell const & fill, std::function<void(Cell*, int)> addToHistory);

    private:

        /** Flag designating the end of line in the buffer. 
         */
        static constexpr char32_t END_OF_LINE = 0x200000;
    }; // ui::AnsiTerminal::Buffer

    // ============================================================================================

    /** Terminal buffer and settings that are specific for each mode (normal vs alternate). 
     */
    class AnsiTerminal::State {
    public:

        State():
            State{Size{80,25}} {
        }

        State(Size size):
            buffer{size},
            canvas{buffer},
            scrollEnd{size.height()},
            lastCharacter_{-1,-1} {
        }

        void reset(Color fg, Color bg) {
            cell = Cell{};
            cell.setFg(fg).setDecor(fg).setBg(bg);
            // reset the cursor
            buffer.setCursorPosition(Point{0,0});
            // reset state
            scrollStart = 0;
            scrollEnd = buffer.height();
            inverseMode = false;
            // clear the buffer
            canvas.fill(Rect{buffer.size()}, cell);
        }

        void resize(Size size, std::function<void(Cell*, int)> addToHistory) {
            buffer.resize(size, cell, addToHistory);
            canvas = Canvas{buffer};
            scrollStart = 0;
            scrollEnd = size.height();
        }

        void setCursor(Point pos) {
            buffer.setCursorPosition(pos);
            // invalidate the last character's position
            lastCharacter_ = Point{-1, -1};
        }

        void saveCursor() {
            cursorStack_.push_back(buffer.cursorPosition());
        }

        void restoreCursor() {
            if (cursorStack_.empty())
                return;
            Point pos = cursorStack_.back();
            cursorStack_.pop_back();
            if (pos.x() >= buffer.size().width())
                pos.setX(buffer.size().width() - 1);
            if (pos.y() >= buffer.size().height())
                pos.setX(buffer.size().height() - 1);
            buffer.setCursorPosition(pos);
        }

        void markLineEnd() {
            buffer.markAsLineEnd(lastCharacter_);
        }

        Buffer buffer;
        Canvas canvas;
        Cell cell;
        int scrollStart{0};
        int scrollEnd;
        bool inverseMode = false;

        // TODO this should be private
        Point lastCharacter_;


    protected:
        std::vector<Point> cursorStack_;
        
    }; // ui::AnsiTerminal::State

    // ============================================================================================

    /** Palette
     */
    class AnsiTerminal::Palette {
    public:

        static Palette Colors16();
        static Palette XTerm256(); 

        Palette():
            size_{2},
            defaultFg_{1},
            defaultBg_{0},
            colors_{new Color[2]} {
            colors_[0] = Color::Black;
            colors_[1] = Color::White;
        }

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

        Palette & operator = (Palette const & other);
        Palette & operator == (Palette && other);

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

    inline Point AnsiTerminal::cursorPosition() const {
        return state_->buffer.cursorPosition();
    }

    inline void AnsiTerminal::setCursorPosition(Point position) {
        state_->buffer.setCursorPosition(position);
    }



} // namespace ui
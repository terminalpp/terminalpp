#pragma once

#include <unordered_map>
#include <unordered_set>

#include "ui/canvas.h"

#include "ui/mixins/selection_owner.h"
#include "ui/special_objects/hyperlink.h"

#include "tpp-lib/pty.h"
#include "tpp-lib/pty_buffer.h"

#include "csi_sequence.h"
#include "osc_sequence.h"

namespace ui {

    class TppSequenceEventPayload {
    public:
        tpp::Sequence::Kind kind;
        char const * payloadStart;
        char const * payloadEnd;
    }; // ui::TppSequenceEventPayload

    using TppSequenceEvent = Event<TppSequenceEventPayload>;

    using ExitCodeEvent = Event<ExitCode>;

    /** The terminal alone. 
     
        The simplest interface to the rerminal, no history, selection, etc?
     */
    class AnsiTerminal : public virtual Widget, public tpp::PTYBuffer<tpp::PTYMaster>, SelectionOwner {
    public:
        using Cell = Canvas::Cell;
        using Cursor = Canvas::Cursor;
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
            // lock the widget for repainting
            Lock l(this);
            // under lock, update history and buffers
            {
                std::lock_guard<PriorityLock> g{bufferLock_.priorityLock(), std::adopt_lock};
                Widget::resize(size);
                resizeHistory();
                resizeBuffers(size);
                pty_->resize(size.width(), size.height());
            }
            if (scrollToTerminal_)
                setScrollOffset(Point{0, historyRows()});
        }

        /** Returns the palette of the terminal. 
         */
        Palette const * palette() const {
            return palette_;
        }

    /** \name Events
     */

    public:
        VoidEvent onNotification;
        StringEvent onHyperlink;
        StringEvent onTitleChange;
        StringEvent onClipboardSetRequest;
        TppSequenceEvent onTppSequence;
        ExitCodeEvent onPTYTerminated;


    /** \name Widget 
     */
    //@{

    protected:

        Size contentsSize() const override {
            if (alternateMode_)
                return Widget::contentsSize();
            else 
                return Size{width(), height() + static_cast<int>(historyRows_.size())};
        }

        void paint(Canvas & canvas) override;

    //@}

    /** \name User Input
     
        Unlike generic widgets, which first process the event themselves and only then trigger the user events or propagate to parents, the terminal first triggers the user event, which may in theory stop the event from propagating to the terminal and only then processes the event in the terminal. No keyboard events propagate to parents. 
     */
    //@{
    public:

        using Widget::requestClipboardPaste;

        using Widget::requestSelectionPaste;

        /** Sends the specified text as clipboard to the PTY. 
         */
        void pasteContents(std::string const & contents);
        
    protected:

        void keyDown(KeyEvent::Payload & e) override ;

        void keyUp(KeyEvent::Payload & e) override;

        void keyChar(KeyCharEvent::Payload & e) override;

        void mouseMove(MouseMoveEvent::Payload & e) override;

        void mouseDown(MouseButtonEvent::Payload & e) override;

        void mouseUp(MouseButtonEvent::Payload & e) override;

        void mouseWheel(MouseWheelEvent::Payload & e) override;

        void mouseClick(MouseButtonEvent::Payload & e) override {
            Widget::mouseClick(e);
            if (e.active()) {
                if (activeHyperlink_ != nullptr) {
                    StringEvent::Payload p{activeHyperlink_->url()};
                    onHyperlink(p, this);
                }
            }
        }

        void mouseOut(VoidEvent::Payload & e) override {
            Widget::mouseOut(e);
            if (activeHyperlink_ != nullptr) {
                activeHyperlink_->setActive(false);
                activeHyperlink_ = nullptr;
            }
        }

        unsigned encodeMouseButton(MouseButton btn, Key modifiers);

        void sendMouseEvent(unsigned button, Point coords, char end);

        std::string getSelectionContents() override;

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

        bool boldIsBright() const {
            return boldIsBright_;
        }

        /** Sets whether bold text is rendered with bright colors automatically. 
         
            The update does not have an immediate effect and the buffer has to be reloaded for the setting to take effect. 
         */
        virtual void setBoldIsBright(bool value = true) {
            boldIsBright_ = value;
        }

        bool displayBold() const {
            return displayBold_;
        }

        /** Determines whether bold text is rendered with bold or normal font. 
         
            The update does not have an immediate effect and the buffer has to be reloaded for the setting to take effect. 
         */
        virtual void setDisplayBold(bool value = true) {
            displayBold_ = value;
        }

        Color inactiveCursorColor() const {
            return inactiveCursorColor_;
        }

        virtual void setInactiveCursorColor(Color value) {
            if (inactiveCursorColor_ != value) {
                inactiveCursorColor_ = value;
                repaint();
            }
        }

        /** Returns the number of current history rows. 
         
            Note that to do so, the buffer must be locked as history rows are protected by its mutex so this function is not as cheap as getting a size of a vector. 
         */
        int historyRows() {
            std::lock_guard<PriorityLock> g{bufferLock_};
            return static_cast<int>(historyRows_.size());
        }

        int maxHistoryRows() const {
            return maxHistoryRows_;
        }

        void setMaxHistoryRows(int value) {
            if (value != maxHistoryRows_) {
                maxHistoryRows_ = std::max(value, 0);
                std::lock_guard<PriorityLock> g{bufferLock_};
                while (historyRows_.size() > static_cast<size_t>(maxHistoryRows_)) {
                    delete [] historyRows_.front().second;
                    historyRows_.pop_front();
                }
            }
        }

    protected:

        void setScrollOffset(Point const & value) override {
            Widget::setScrollOffset(value);
            scrollToTerminal_ = value.y() == historyRows();
        }

        /** Returns current cursor position. 
         */
        Point cursorPosition() const;

        void setCursorPosition(Point position);

        /** Returns the current cursor. 
         */
        Cursor & cursor();

        /** Inserts given number of lines at given top row.
            
            Scrolls down all lines between top and bottom accordingly. Fills the new lines with the provided cell.
        */
        void insertLines(int lines, int top, int bottom, Cell const & fill);    

        /** Deletes lines and triggers the onLineScrolledOut event if appropriate. 
         
            The event is triggered only if the terminal is in normal mode and if the scroll region starts at the top of the window. 
            */
        void deleteLines(int lines, int top, int bottom, Cell const & fill);

        void addHistoryRow(Cell * row, int cols);

        void ptyTerminated(ExitCode exitCode) override {
            schedule([this, exitCode](){
                ExitCodeEvent::Payload p{exitCode};
                onPTYTerminated(p, this);
            });
        }

        void resizeHistory();
        void resizeBuffers(Size size);


        // TODO change to int
        void deleteCharacters(unsigned num);
        void insertCharacters(unsigned num);

        /** Returns the top offset of the terminal buffer in the currently drawed. 
         */
        int terminalBufferTop() {
            ASSERT(bufferLock_.locked());
            return alternateMode_ ? 0 : static_cast<int>(historyRows_.size());            
        }

        /** Converts the given widget coordinates to terminal buffer coordinates. 
         */
        Point toBufferCoords(Point const & widgetCoordinates) {
            ASSERT(bufferLock_.locked());
            return widgetCoordinates + scrollOffset() - Point{0, terminalBufferTop()};
        }

        /** Returns the cell at given coordinates. 
         
            The coordinates are adjusted for the scroll buffer and then either a terminal buffer, or history cell is returned. In case of history cells, it is possible that no cell exists at the coordinates if the particular line was terminated before, in which case nullptr is returned. 
         */
        Cell const * cellAt(Point widgetCoords);

        /** Returns hyperlink attached to cell at given coordinates. 
         
            If there are no cells, at given coordinates, or no hyperlink present, returns nullptr.
         */
        Hyperlink * hyperlinkAt(Point widgetCoords) {
            Cell const * cell = cellAt(widgetCoords);
            return cell == nullptr ? nullptr : dynamic_cast<Hyperlink*>(cell->specialObject());
        }

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

        /** If true, bold font will be displayed. */
        bool displayBold_ = true;

        /** Determines whether alternate mode is active or not. */
        bool alternateMode_ = false;

        /** On when the terminal is scrolled completely in view (i.e. past all history rows) and any history rows added will automatically scroll the terminal as well. 
         */
        bool scrollToTerminal_ = true;

        /** Current state and its backup. The states are swapped and current state kind is determined by the alternateMode(). 
         */
        State * state_;
        State * stateBackup_;
        PriorityLock bufferLock_;

        int maxHistoryRows_;
        std::deque<std::pair<int, Cell*>> historyRows_;

        /** When hyperlink is parsed, this holds the special object and the offset of the next cell. If the hyperlink in progress is nullptr, then there is no hyperlink in progress and hyperlink offset has no meaning. 
         */
        Hyperlink::Ptr inProgressHyperlink_ = nullptr;

        /** Hyperlink activated by mouse hover. 
         */
        Hyperlink::Ptr activeHyperlink_ = nullptr;

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
        virtual void tppSequence(TppSequenceEvent::Payload seq) {
            onTppSequence(seq, this);
        }

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
        friend class AnsiTerminal;
    public:
        Buffer(Size const & size, Cell const & defaultCell):
            ui::Canvas::Buffer(size) {
            fill(defaultCell);
        }

        void insertLine(int top, int bottom, Cell const & fill);

        std::pair<Cell *, int> copyRow(int row, Color defaultBg);

        void deleteLine(int top, int bottom, Cell const & fill);

        void markAsLineEnd(Point p) {
            if (p.x() >= 0)
                SetUnusedBits(at(p), END_OF_LINE);
        }

        static bool IsLineEnd(Cell const & c) {
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

        /** Fills the entire terminal withe the given cell. 
         */
        void fill(Cell const& defaultCell) {
            for (int i = 0, e = height(); i < e; ++i)
                fillRow(i, defaultCell, 0, width());
        }
        

        void resize(Size size, Cell const & fill, std::function<void(Cell*, int)> addToHistory);

    private:

        Cell * row(int row) {
            ASSERT(row >= 0 && row < height());
            return rows_[row];
        }

        /** Flag designating the end of line in the buffer. 
         */
        static constexpr char32_t END_OF_LINE = 0x200000;
    }; // ui::AnsiTerminal::Buffer

    // ============================================================================================

    /** Terminal buffer and settings that are specific for each mode (normal vs alternate). 
     */
    class AnsiTerminal::State {
    public:

        State(Color defaultBackground):
            State{Size{80,25}, defaultBackground} {
        }

        State(Size size, Color defaultBackground):
            buffer{ size, Cell{}.setBg(defaultBackground) },
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
            invalidateLastCharacter();
        }

        void saveCursor() {
            cursorStack_.push_back(buffer.cursorPosition());
        }

        void restoreCursor() {
            if (cursorStack_.empty())
                return;
            Point pos = cursorStack_.back();
            cursorStack_.pop_back();
            if (pos.x() >= buffer.width())
                pos.setX(buffer.width() - 1);
            if (pos.y() >= buffer.height())
                pos.setX(buffer.height() - 1);
            buffer.setCursorPosition(pos);
        }

        void markLineEnd() {
            buffer.markAsLineEnd(lastCharacter_);
        }

        void invalidateLastCharacter() {
            lastCharacter_ = Point{1,1};
        }

        void setLastCharacter(Point p) {
            lastCharacter_ = p;
        }

        Buffer buffer;
        Canvas canvas;
        Cell cell;
        int scrollStart{0};
        int scrollEnd;
        bool inverseMode = false;

        /** When bold is bright *and* bold is not displayed this is the only way to determine whether to use bright colors because the cell's font is not bold. 
         */
        bool bold = false;


    protected:
        Point lastCharacter_;
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
            defaultFg_{Color::White},
            defaultBg_{Color::Black},
            colors_{new Color[2]} {
            colors_[0] = Color::Black;
            colors_[1] = Color::White;
        }

        Palette(size_t size, Color defaultFg = Color::White, Color defaultBg = Color::Black):
            size_(size),
            defaultFg_(defaultFg),
            defaultBg_(defaultBg),
            colors_(new Color[size]) {
        }

        Palette(std::initializer_list<Color> colors, Color defaultFg = Color::White, Color defaultBg = Color::Black);

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
            return defaultFg_;
        }

        Color defaultBackground() const {
            return defaultBg_;
        }

        void setDefaultForeground(size_t index) {
            defaultFg_ = colors_[index];
        }

        void setDefaultForeground(Color color) {
            defaultFg_ = color;
        }

        void setDefaultBackground(size_t index) {
            defaultBg_ = colors_[index];
        }

        void setDefaultBackground(Color color) {
            defaultBg_ = color;
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
        Color defaultFg_;
        Color defaultBg_;
        Color * colors_;

    }; // AnsiTerminal::Palette

    inline Point AnsiTerminal::cursorPosition() const {
        return state_->buffer.cursorPosition();
    }

    inline void AnsiTerminal::setCursorPosition(Point position) {
        state_->buffer.setCursorPosition(position);
    }

    inline AnsiTerminal::Cursor & AnsiTerminal::cursor() {
        return state_->buffer.cursor();
    }

} // namespace ui
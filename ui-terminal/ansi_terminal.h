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
#include "url_matcher.h"

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
    class AnsiTerminal : public virtual Widget, public tpp::PTYBuffer<tpp::PTYMaster> {
    public:

        using Cell = Canvas::Cell;
        using Cursor = Canvas::Cursor;

        /** Palette
         */
        class Palette {
        public:
            static Palette Colors16();
            static Palette XTerm256(); 

            Palette():
                defaultFg_{Color::White},
                defaultBg_{Color::Black},
                colors_{{Color::Black, Color::White}} {
            }

            Palette(size_t size, Color defaultFg = Color::White, Color defaultBg = Color::Black):
                defaultFg_(defaultFg),
                defaultBg_(defaultBg) {
                colors_.resize(size);
            }

            Palette(std::initializer_list<Color> colors, Color defaultFg = Color::White, Color defaultBg = Color::Black);

            Palette(Palette const & from) = default;

            Palette(Palette && from) noexcept = default;


            Palette & operator = (Palette const & other) = default;
            Palette & operator = (Palette && other) noexcept = default;

            size_t size() const {
                return colors_.size();
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

            Color operator [] (size_t index) const {
                ASSERT(index < size());
                return colors_[index];
            } 

            Color & operator [] (size_t index) {
                ASSERT(index < size());
                return colors_[index];
            } 

        private:

            Color defaultFg_;
            Color defaultBg_;
            std::vector<Color> colors_;

        }; // ui::AnsiTerminal::Palette

        enum class BufferKind {
            Normal,
            Alternate,
        };

        /** The terminal buffer is a canvas buffer augmented with terminal specific properties such as the scroll window. 
         
            The buffer is tightly coupled with its terminal. 
         */
        class Buffer : public Canvas::Buffer {
            friend class AnsiTerminal;
        public:
            Buffer(BufferKind bufferKind, AnsiTerminal * terminal, Cell const & defaultCell):
                Canvas::Buffer{MinSize(terminal->size())},
                bufferKind_{bufferKind},
                terminal_{terminal},
                currentCell_{defaultCell},
                scrollEnd_{height()} {
                fill(currentCell_);
            }

            void reset(Color fg, Color bg) {
                currentCell_ = Cell{}.setFg(fg).setDecor(fg).setBg(bg);
                scrollStart_ = 0;
                scrollEnd_ = height();
                inverseMode_ = false;
                setCursorPosition(Point{0,0});
                fill(currentCell_);
            }

            Cell const & currentCell() const {
                return currentCell_;
            }

            Cell & currentCell() {
                return currentCell_;
            }

            int scrollStart() const {
                return scrollStart_;
            }

            int scrollEnd() const {
                return scrollEnd_;
            }

            void setScrollRegion(int start, int end) {
                scrollStart_ = start;
                scrollEnd_ = end;
            }

            /** Resizes the buffer and updates the state accordingly. 
             
                When called, the terminal must already be resized. 
             */
            void resize();

            /** Writes given character at the cursor position using the current cell attributes and returns the cell itself. 
             */
            Cell & addCharacter(char32_t codepoint);

            /** Adds new line. 
             
                Does not add any visible character, but marks the last character position, if valid as line end and updates the cursor position accordingly. 
             */
            void newLine();

            void carriageReturn() {
                cursorPosition_.setX(0);
            }

            /** Inserts N characters right of the position, shifting the rest of the line appropriately to the right. 
             
                Characters that would end up right of the end of the buffer are discarded. 
             */
            void insertCharacters(Point from, int num) {
                ASSERT(from.x() + num <= width() && num >= 0);
                // first copy the characters
                for (int c = width() - 1, e = from.x() + num; c >= e; --c)
                    at(c, from.y()) = at(c - num, from.y());
                for (int c = from.x(), e = from.x() + num; c < e; ++c)
                    at(c, from.y()) = currentCell_;
            }

            /** Deletes N characters right of the position, shifts the line to the left accordingly and fills the rightmost cells with given current cell. 
             */
            void deleteCharacters(Point from, int num) {
                for (int c = from.x(), e = width() - num; c < e; ++c) 
                    at(c, from.y()) = at(c + num, from.y());
                for (int c = width() - num, e = width(); c < e; ++c)
                    at(c, from.y()) = currentCell_;
            }

            void insertLine(int top, int bottom, Cell const & fill);

            /** TODO this can be made faster and the lines can be moved only once. 
             */
            void insertLines(int lines, int top, int bottom, Cell const & fill) {
                while (lines-- > 0)
                    insertLine(top, bottom, fill);
            }

            /** Deletes given line and scrolls all lines in the given range (top-bottom) one line up, filling the new line at the bottom with the given cell. 
             
                Triggers the terminal's onNewHistoryLine event. 
             */
            void deleteLine(int top, int bottom, Cell const & fill);

            void deleteLines(int lines, int top, int bottom, Cell const & fill) {
                while (lines-- > 0)
                    deleteLine(top, bottom, fill);
            }

            /** Adjusts the cursor position to remain within the buffer and shifts the buffer if necessary. 
             
                Makes sure that cursor position is within the buffer bounds. If the cursor is too far left, it is reset on next line, first column. If the cursor is below the buffer the buffer is scrolled appropriately. 

                Returns the cursor position after the adjustment, which is guaranteed to be always valid and within the buffer bounds.
            */
            Point adjustedCursorPosition();

            /** Overrides canvas cursor position to disable the check whether the cell has the cursor flag. 
             
                The cursor in terminal is only one and always valid at the coordinates specified in the buffer. 
                
                Note that the cursor position may *not* be valid within the buffer (cursor after text can be advanced beyond the buffer itself). To obtain a valid cursor position, which may shift the buffer contents, use the adjustCursorPosition function instead. 
            */
            Point cursorPosition() const {
                return cursorPosition_;
            }

            void setCursorPosition(Point pos) {
                cursorPosition_ = pos;
                invalidateLastCharacter();
            }

            // TODO advance cursor that is faster that setCursorPosition

            void setCursor(Canvas::Cursor const & value, Point position) {
                cursor_ = value;
                setCursorPosition(position);
            }

            void saveCursor() {
                cursorStack_.push_back(cursorPosition());
            }

            void restoreCursor() {
                if (cursorStack_.empty())
                    return;
                Point pos = cursorStack_.back();
                ASSERT(pos.x() >= 0 && pos.y() >= 0);
                cursorStack_.pop_back();
                if (pos.x() >= width())
                    pos.setX(width() - 1);
                if (pos.y() >= height())
                    pos.setX(height() - 1);
                setCursorPosition(pos);
            }

            void resetAttributes();

            bool isBold() const {
                return bold_;
            }

            void setBold(bool value);

            void setInverseMode(bool value) {
                if (inverseMode_ == value)
                    return;
                inverseMode_ = value;
                Color fg = currentCell_.fg();
                Color bg = currentCell_.bg();
                currentCell_.setFg(bg).setDecor(bg).setBg(fg);
            }

            Canvas canvas() {
                return Canvas{*this};
            }

        private:

            /** Returns the start of the line that contains the cursor including any word wrap. 
             
                I.e. if the cursor is on line that started 3 lines above and was word-wrapped to the width of the terminal returns the current cursor row minus three. 
            */
            int getCursorRowWrappedStart() const;

            /** Returns true if the given line contains only whitespace characters from given column to its width. 
             
                If the column start is greater or equal to the width, returns true. 
            */
            bool hasOnlyWhitespace(Cell * row, int from, int width);

            void markAsLineEnd(Point p) {
                if (p.x() >= 0)
                    SetUnusedBits(at(p), END_OF_LINE);
            }

            /** Invalidates the cached location of last printable character for end of line detection in the terminal. 
             
                Typically occurs during forceful cursor position updates, etc. 
             */
            void invalidateLastCharacter() {
                lastCharacter_ = Point{-1, -1};
            }

            static bool IsLineEnd(Cell const & c) {
                return GetUnusedBits(c) & END_OF_LINE;
            }

            /** Prevents terminal buffer of size 0 to be instantiated. 
             
                TODO should this move to Canvas::Buffer? 
             */
            static Size MinSize(Size request) {
                if (request.width() <= 0)
                    request.setWidth(1);
                if (request.height() <= 0)
                    request.setHeight(1);
                return request;
            }

            BufferKind bufferKind_;
            AnsiTerminal * terminal_;

            Cell currentCell_;
            int scrollStart_ = 0;
            int scrollEnd_;
            bool inverseMode_ = false;

            /** When bold is bright *and* bold is not displayed this is the only way to determine whether to use bright colors because the cell's font is not bold. 
             */
            bool bold_ = false;

            Point lastCharacter_ = Point{-1, -1};
            std::vector<Point> cursorStack_;

            /** Flag designating the end of line in the buffer. 
             */
            static constexpr char32_t END_OF_LINE = 0x200000;

        }; // ui::AnsiTerminal::Buffer

        using BufferChangeEvent = Event<BufferKind>;

        struct HistoryRow {
        public:
            BufferKind buffer;
            int width;
            Cell const * cells;
        };

        using NewHistoryRowEvent = Event<HistoryRow>;

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

    protected:



      
        /** Triggered when the terminal enables, or disables an alternate mode. 
         
            Can be called from any thread, expects the terminal buffer lock.
         */
        BufferChangeEvent onBufferChange;

        /** Triggered when new row is evicted from the terminal's scroll region. 
         
            Can be triggered from any thread, expects the terminal buffer lock. The cells will be reused by the terminal after the call returns. 
         */
        NewHistoryRowEvent onNewHistoryRow;

    public:
        AnsiTerminal(tpp::PTYMaster * pty, Palette && palette);

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
                buffer_.resize();
                bufferBackup_.resize();
                pty_->resize(size.width(), size.height());
            }
/*            
            if (scrollToTerminal_)
                setScrollOffset(Point{0, historyRows()});
                */
        }

        /** Returns the palette of the terminal. 
         */
        Palette const & palette() const {
            return palette_;
        }

    /** \name Events
     */

    public:
        VoidEvent onNotification;
        
        /** Triggered when user clicks on a hyperlink with left mouse button. 
         
            The default action should be opening the hyperlink. 
         */
        StringEvent onHyperlinkOpen;

        /** Triggered when user clicks on a hyperlink with right mouse button. 
         
            The default action should be copying the url to clipboard.
         */
        StringEvent onHyperlinkCopy;
        StringEvent onTitleChange;
        StringEvent onClipboardSetRequest;
        TppSequenceEvent onTppSequence;
        ExitCodeEvent onPTYTerminated;


    /** \name Widget 
     */
    //@{

    protected:

        void paint(Canvas & canvas) override;

    //@}

    /** \name User Input
     
        Unlike generic widgets, which first process the event themselves and only then trigger the user events or propagate to parents, the terminal first triggers the user event, which may in theory stop the event from propagating to the terminal and only then processes the event in the terminal. No keyboard events propagate to parents. 
     */
    //@{
    public:

        using Widget::requestClipboardPaste;

        using Widget::requestSelectionPaste;

        /** Returns true if the application running in the terminal captures mouse events.
         */
        bool mouseCaptured() const {
            std::lock_guard<PriorityLock> g{bufferLock_.priorityLock(), std::adopt_lock};
            return mouseMode_ != MouseMode::Off;
        }

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

        /** Mouse click. 
         
            Left button open active hyperlink, right button copies the hyperlink's url to clipboard. Both work only if the app in the terminal does not capture mouse. 
         */
        void mouseClick(MouseButtonEvent::Payload & e) override {
            Widget::mouseClick(e);
            if (! mouseCaptured() && e.active()) {
                if (activeHyperlink_ != nullptr) {
                    StringEvent::Payload p{activeHyperlink_->url()};
                    if (e->button == MouseButton::Left)
                        onHyperlinkOpen(p, this);
                    else if (e->button == MouseButton::Right)
                        onHyperlinkCopy(p, this);
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


    private:

        /** Number of pressed mouse buttons to determine mouse capture. */
        unsigned mouseButtonsDown_ = 0;
        /** Last pressed mouse button for mouse move reporting. */
        unsigned mouseLastButton_ = 0;

        static std::unordered_map<Key, std::string> KeyMap_;
        static std::unordered_set<Key> PrintableKeys_;

    //@}

    /** \name Hyperlinks
     
        Terminal supports hyperlinks either via the OSC 8 sequence, or by automatic detection based on the UrlMatcher class. Cell special objects are used to store the information about a hyperlink. 

        The sequence creates the hyperlink, which is then attached to each next cell untiln the closing empty sequence is found. The size of the parameters or the link itself is not limited in any way on the terminal, but the maximum input buffer size will limit this in practice as the whole OSC sequence must fit in the buffer. 

        When codepoints are sent to the terminal, they are matched continuously whether they match against an url, in which case a hyperlink is created and attached to the cells as well. 

        When mouse hovers over the cells with attached hyperlink, the link is highighted. When clicked, the onHyperlinkOpen event is triggered, which should open the link in a browser. On right button clicik, the onHyperlinkCopy event is triggered, which should copy the url to clipboard instead of opening. 
     */
    //@{
    public:

        /** Returns whether the OSC hyperlink extension (OSC 8) is enabled. 
         */
        bool allowOSCHyperlinks() const {
            return allowOSCHyperlinks_;
        }

        /** Enables, or disables the OSC hyperlink extension. 
         
            Note that disabling OSC hyperlinks has no effect on the automatic hyperlink detection.
         */
        virtual void setAllowOSCHyperlinks(bool value = true) {
            allowOSCHyperlinks_ = value;
        }

        /** Returns true if hyperlinks are detected within normal text in the terminal. 
         */
        bool detectHyperlinks() const {
            return detectHyperlinks_;
        }

        /** Istructs the terminal to turn automatic hyperlink detection on or off.
         
            Note that disabling automatic hyperlink detection has no effect on OSC explicit hyperlinks.
         */
        virtual void setDetectHyperlinks(bool value = true) {
            detectHyperlinks_ = value;
            urlMatcher_.reset();
        }

        /** Returns the style used for new hyperlinks. 
         */
        Hyperlink::Style const & normalHyperlinkStyle() const {
            return normalHyperlinkStyle_;
        }

        /** Sets the style for new hyperlinks.
         */
        virtual void setNormalHyperlinkStyle(Hyperlink::Style const & value) {
            normalHyperlinkStyle_ = value;
        }

        /** Returns the active (mouse over) style for new hyperlinks. 
         */
        Hyperlink::Style const & activeHyperlinkStyle() const {
            return activeHyperlinkStyle_;
        }

        /** Sets the active (mouse over) style for new hyperlinks.
         */
        virtual void setActiveHyperlinkStyle(Hyperlink::Style const & value) {
            activeHyperlinkStyle_ = value;
        }

    protected:

        /** Resets the hyperlink detection matching. 
         
            If the matching is in valid state, creates the hyperlink. The reset is done by adding an url separator character, which terminates the url scheme.

            Any sequence other than SGR ones should reset the hyperlink matching.
         */
        void resetHyperlinkDetection() {
            detectHyperlink(' ');
        }

        /** Adds given character to the url matcher. 
         
            If adding the character makes a previously valid match invalid, creates the hyperlink and attaches it to its respective cells.
         */
        void detectHyperlink(char32_t next);

    private:

        /** When hyperlink is parsed, this holds the special object and the offset of the next cell. If the hyperlink in progress is nullptr, then there is no hyperlink in progress and hyperlink offset has no meaning. 
         */
        Hyperlink::Ptr inProgressHyperlink_;

        /** Hyperlink activated by mouse hover. 
         */
        Hyperlink::Ptr activeHyperlink_;

        /** Url matcher to detect hyperlinks in the terminal output automatically. 
         */
        UrlMatcher urlMatcher_;

        /** If true, OSC hyperlinks are supported. 
         */
        bool allowOSCHyperlinks_ = false;

        /** If true, hyperlinks are autodetected. 
         */
        bool detectHyperlinks_ = false;

        /** Default hyperlink style (for both autodetected and explicit OSC 8 links).
         */
        Hyperlink::Style normalHyperlinkStyle_;

        /** Active hyperlink style (for both autodetected and explicit OSC 8 links), used when mouse is over the link.
         */
        Hyperlink::Style activeHyperlinkStyle_;

    //@}


    /** \name Terminal State and scrollback buffer
     */
    //@{
    
    public:

        /** Returns the default empty cell of the terminal. 
         */
        Cell defaultCell() const {
            return Cell{}.setFg(palette_.defaultForeground()).setDecor(palette_.defaultForeground()).setBg(palette_.defaultBackground());
        }

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

        /** Returns true if terminal applications can change cursor behavior. 
         
            Note that the terminal apps can always set cursor visibility. 
         */
        bool allowCursorChanges() const {
            return allowCursorChanges_;
        }

        /** Updates whether terminal applications can change cursor behavior.
         
            Note that the terminal apps can always set cursor visibility. 
         */
        virtual void setAllowCursorChanges(bool value) {
            allowCursorChanges_ = value;
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

        /** Sets the cursor appearance.
         
            The cursor is set for both normal and alternate buffers. If the application in the terminal chooses to rewrite the cursor settings, it can do so. 
         */
        void setCursor(Canvas::Cursor const & value) {
            buffer_.cursor() = value;
            bufferBackup_.cursor() = value;
        }


    protected:

        Locked<Buffer const> buffer() const {
            return Locked{buffer_, bufferLock_.priorityLock(), std::adopt_lock};
        }

        Locked<Buffer> buffer() {
            return Locked{buffer_, bufferLock_.priorityLock(), std::adopt_lock};
        }

        void ptyTerminated(ExitCode exitCode) override {
            schedule([this, exitCode](){
                ExitCodeEvent::Payload p{exitCode};
                onPTYTerminated(p, this);
            });
        }

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

        Palette palette_;

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

        /** If true, terminal apps can change cursor behavior.
         */
        bool allowCursorChanges_ = true;

        /** Current buffer and its backup. 
         
            The states are swapped and current state kind is determined by the alternateMode(). The alternate buffer is not supposed to be accessed at all and the normal buffer is protected by the bufferLock_. 
         */
        Buffer buffer_;
        Buffer bufferBackup_;
        mutable PriorityLock bufferLock_;


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

    inline void AnsiTerminal::Buffer::resetAttributes() {
        currentCell_ = terminal_->defaultCell();
        bold_ = false;
        inverseMode_ = false;
    }

    inline void AnsiTerminal::Buffer::setBold(bool value) {
        bold_ = value;
        if (!value || terminal_->displayBold_)
            currentCell_.font().setBold(value);
    }

#ifdef FOOBAR

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


        void resize(Size size, Cell const & fill, std::function<void(Cell*, int)> addToHistory);

    private:

        Cell * row(int row) {
            ASSERT(row >= 0 && row < height());
            return rows_[row];
        }

        /** Returns the start of the line that contains the cursor including any word wrap. 
         
            I.e. if the cursor is on line that started 3 lines above and was word-wrapped to the width of the terminal returns the current cursor row minus three. 
         */
        int getCursorRowWrappedStart() const;

        /** Adjusts the cursor position to remain within the buffer and shifts the buffer if necessary. 
         
            Makes sure that cursor position is within the buffer bounds. If the cursor is too far left, it is reset on next line, first column. If the cursor is below the buffer the buffer is scrolled appropriately. 

            TODO can this be used by the terminal cursor positioning, perhaps by making sure it works on more than + 1 offsets outside the valid bounds? And also scroll region and so on...
         */
        void adjustCursorPosition(Cell const & fill, std::function<void(Cell*, int)> addToHistory);
        
        /** Returns true if the given line contains only whitespace characters from given column to its width. 
         
            If the column start is greater or equal to the width, returns true. 
         */
        bool hasOnlyWhitespace(Cell * row, int from, int width);


    }; // ui::AnsiTerminal::Buffer

    // ============================================================================================

    /** Terminal buffer and settings that are specific for each mode (normal vs alternate). 
     */
    class AnsiTerminal::State {
    public:

        explicit State(Color defaultBackground):
            State{Size{80,25}, defaultBackground} {
        }

        State(Size size, Color defaultBackground):
            buffer{ size, Cell{}.setBg(defaultBackground) },
            canvas{buffer},
            scrollEnd{size.height()} {
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
        Point lastCharacter_ = Point{-1,-1};
        std::vector<Point> cursorStack_;
        
    }; // ui::AnsiTerminal::State

#endif

    // ============================================================================================

    /*
    inline Point AnsiTerminal::cursorPosition() const {
        return buffer_.cursorPosition();
    }

    inline void AnsiTerminal::setCursorPosition(Point position) {
        state_->buffer.setCursorPosition(position);
    }

    inline AnsiTerminal::Cursor & AnsiTerminal::cursor() {
        return state_->buffer.cursor();
    }
    */

} // namespace ui
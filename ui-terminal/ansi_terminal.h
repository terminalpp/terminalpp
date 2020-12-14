#pragma once

#include <unordered_map>
#include <unordered_set>

#include "ui/canvas.h"

#include "ui/mixins/selection_owner.h"
#include "ui/special_objects/hyperlink.h"

#include "tpp-lib/pty.h"
#include "tpp-lib/pty_buffer.h"

#include "terminal.h"
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
    class AnsiTerminal : public Terminal, public tpp::PTYBuffer<tpp::PTYMaster> {
    public:




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
                // resize with the buffer, not the widget as the buffer implements its own size policy
                pty_->resize(buffer_.width(), buffer_.height());
            }
/*            
            if (scrollToTerminal_)
                setScrollOffset(Point{0, historyRows()});
                */
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


} // namespace ui
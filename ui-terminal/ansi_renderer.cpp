#include "helpers/char.h"
#include "helpers/ansi_sequences.h"

#include "ui/event_queue.h"

#include "ansi_renderer.h"

namespace ui {

    namespace {

        void InitializeVTKeys(MatchingFSM<Key, char> & keys) {
#define KEY(K, ...) { std::string x = STR(__VA_ARGS__); keys.addMatch(x.c_str(), K, /* override */ true); }
#include "ansi_keys.inc.h"
            // this is a hack, correctly matching invalid key does match the SGR mouse encoding
            keys.addMatch("\033[<", Key::Invalid); 
        }
    }

    MatchingFSM<Key, char> AnsiRenderer::VtKeys_;

    AnsiRenderer::AnsiRenderer(tpp::PTYSlave * pty, EventQueue & eventQueue):
        Renderer{pty->size(), eventQueue},
        tpp::TerminalClient{pty} {
        if (VtKeys_.empty())
            InitializeVTKeys(VtKeys_);
        // enable SGR mouse reporting and report all movements
        send("\033[?1003;1006h", 13);
    }

    AnsiRenderer::~AnsiRenderer() {
        // disable mouse reporting & reset mouse encoding to default
        send("\033[?1003;1006l", 13);
    }

    void AnsiRenderer::render(Rect const & rect) {
        std::stringstream s;
        // shorthand to the buffer
        Buffer const & buffer = this->buffer();
        // initialize the state
        Cell state = buffer.at(rect.topLeft());
        s << ansi::SGRReset() 
            << ansi::Fg(state.fg().r, state.fg().g, state.fg().b)
            << ansi::Bg(state.bg().r, state.bg().g, state.bg().b);
        if (state.font().bold())
            s << ansi::Bold();
        if (state.font().italic())
            s << ansi::Italic();
        if (state.font().underline())
            s << ansi::Underline();
        if (state.font().strikethrough())
            s << ansi::Strikethrough();
        if (state.font().blink())
            s << ansi::Blink();
        // actually output the buffer
        for (int y = rect.top(), ye = rect.bottom(); y < ye; ++y) {
            // for each row, first set the cursor properly
            int x = rect.left();
            s << ansi::SetCursor(x, y);
            // then for each cell update the attributes & colors if needs be and output the cell
            for (int xe = rect.right(); x < xe; ++x) {
                Cell const & c = buffer.at(x, y);
                if (c.fg() != state.fg()) {
                    state.setFg(c.fg());
                    s << ansi::Fg(state.fg().r, state.fg().g, state.fg().b);
                }
                if (c.bg() != state.bg()) {
                    state.setBg(c.bg());
                    s << ansi::Bg(state.bg().r, state.bg().g, state.bg().b);
                }
                if (c.font().bold() != state.font().bold()) {
                    state.font().setBold(c.font().bold());
                    s << ansi::Bold(state.font().bold());
                }
                if (c.font().italic() != state.font().italic()) {
                    state.font().setItalic(c.font().italic());
                    s << ansi::Italic(state.font().italic());
                }
                if (c.font().underline() != state.font().underline()) {
                    state.font().setUnderline(c.font().underline());
                    s << ansi::Underline(state.font().underline());
                }
                if (c.font().strikethrough() != state.font().strikethrough()) {
                    state.font().setStrikethrough(c.font().strikethrough());
                    s << ansi::Strikethrough(state.font().strikethrough());
                }
                if (c.font().blink() != state.font().blink()) {
                    state.font().setBlink(c.font().blink());
                    s << ansi::Blink(state.font().blink());
                }
                // finally output the codepoint
                s << Char{c.codepoint()};
            }
        }
        // TODO this is a very unnecessary copy, should be fixed
        std::string x{s.str()};
        send(x.c_str(), x.size());
    }

    /** Non-tpp input sequences can be either mouse, or keyboard input. 
     */
    size_t AnsiRenderer::received(char const * buffer, char const * bufferEnd) {
        char const * processed = buffer;
        while (processed != bufferEnd) {
            char const * i = processed;
            Key k;
            // first see if we can match the beginning of a buffer to known key, in which case keyDown is to be emitted
            if (VtKeys_.match(i, bufferEnd, k)) {
                if (k != Key::Invalid) {
                    keyDown(k);
                } else {
                    CSISequence seq = CSISequence::Parse(processed, bufferEnd);
                    // if the sequence is not valid, processed has been advanced, but the seq should be ignored
                    if (!seq.valid())
                        continue;
                    // if the sequence is not complete, wait for more data
                    if (!seq.complete())
                        break;
                    // parse the sequence and then continue as the processed counter has already been advanced
                    parseSequence(seq);
                    continue;
                }
            }
            // if there is not enough for a valid utf8 character, break
            Char::iterator_utf8 it{processed};
            if (processed + it.charSize() > bufferEnd)
                break;
            Char c{*it};
            if (Char::IsPrintable(c.codepoint()))
                keyChar(c);
            processed += it.charSize();
            if (processed < i)
                processed = i;    
        }
        return processed - buffer;

/*        for (char const * i = buffer; i != bufferEnd; ++i)
            if (*i == '\003') // Ctrl + C
#if (defined ARCH_UNIX)
                raise(SIGINT);
#else
                exit(EXIT_FAILURE);
#endif
        return 0;
    */
    }

    void AnsiRenderer::parseSequence(CSISequence const & seq) {
        switch (seq.firstByte()) {
            case '<': // SGR mouse 
                parseSGRMouse(seq);
                break;
            default:
                // TODO log invalid sequence
                break;
        }
    }

    /** The encoding is: \033[< button ; x ; y END
                        
        Where the button means 0 = left, 1 = right, 2 = wheel. 4 == shift, 8 = alt , 16 = ctrl
        64 = mouse wheel
        32 = mouse move

        END = M = mouse Down, Wheel
        m = mouse up

     */
    void AnsiRenderer::parseSGRMouse(CSISequence const & seq) {
        if (seq.numArgs() != 3) {
            // TODO log the error format
            return;
        }
        // determine the mouse button
        MouseButton button = MouseButton::Left;
        if (seq[0] & 1)
            button = MouseButton::Right;
        else if (seq[0] & 2)
            button = MouseButton::Wheel;
        // update the modifiers based on the button value, but don't emit the key up or down events as they would be mis timed to mouse move as opposed to the actual key press
        Key m;
        if (seq[0] & 4)
            m += Key::Shift;
        if (seq[0] & 8)
            m += Key::Alt;
        if (seq[0] & 16)
            m += Key::Ctrl;
        if (modifiers() & Key::Win)
            m += Key::Win;
        setModifiers(m);
        // and the coordinates, update them to 0-indexed values
        Point coords{seq[1] - 1, seq[2] - 1} ;
        // now determine the type of event
        if (seq[0] & 64) { // mouse wheel
            switch (button) {
                case MouseButton::Left:
                    mouseWheel(coords, 1);
                    break;
                case MouseButton::Right:
                    mouseWheel(coords, 2);
                    break;
                default:
                    break; // invalid encoding
            }
        } else if (seq[0] & 32) {
            mouseMove(coords);
        } else if (seq.finalByte() == 'M') {
            mouseDown(coords, button);
        } else if (seq.finalByte() == 'm') {
            mouseUp(coords, button);
        }
    }

    void AnsiRenderer::receivedSequence(tpp::Sequence::Kind, char const * buffer, char const * bufferEnd) {
        MARK_AS_UNUSED(buffer);
        MARK_AS_UNUSED(bufferEnd);
        NOT_IMPLEMENTED;
    }

} // namespace ui
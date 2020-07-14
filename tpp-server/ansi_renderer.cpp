#include "helpers/char.h"
#include "helpers/ansi_sequences.h"

#include "ansi_renderer.h"

namespace ui3 {

    namespace {

        void InitializeVTKeys(MatchingFSM<Key, char> & keys) {
#define KEY(K, ...) { std::string x = STR(__VA_ARGS__); keys.addMatch(x.c_str(), x.c_str() + x.size(), K, /* override */ true); }
#include "ansi_keys.inc.h"
        }

    }

    MatchingFSM<Key, char> AnsiRenderer::VtKeys_;

    AnsiRenderer::AnsiRenderer(tpp::PTYSlave * pty):
        Renderer{pty->size()},
        tpp::TerminalClient{pty} {
        if (VtKeys_.empty())
            InitializeVTKeys(VtKeys_);
    }

    AnsiRenderer::~AnsiRenderer() {
        
    }

    void AnsiRenderer::render(Buffer const & buffer, Rect const & rect) {
        std::stringstream s;
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
                    state.fg() = c.fg();
                    s << ansi::Fg(state.fg().r, state.fg().g, state.fg().b);
                }
                if (c.bg() != state.bg()) {
                    state.bg() = c.bg();
                    s << ansi::Bg(state.bg().r, state.bg().g, state.bg().b);
                }
                if (c.font().bold() != state.font().bold()) {
                    state.setFont(state.font().setBold(c.font().bold()));
                    s << ansi::Bold(state.font().bold());
                }
                if (c.font().italic() != state.font().italic()) {
                    state.setFont(state.font().setItalic(c.font().italic()));
                    s << ansi::Italic(state.font().italic());
                }
                if (c.font().underline() != state.font().underline()) {
                    state.setFont(state.font().setUnderline(c.font().underline()));
                    s << ansi::Underline(state.font().underline());
                }
                if (c.font().strikethrough() != state.font().strikethrough()) {
                    state.setFont(state.font().setStrikethrough(c.font().strikethrough()));
                    s << ansi::Strikethrough(state.font().strikethrough());
                }
                if (c.font().blink() != state.font().blink()) {
                    state.setFont(state.font().setBlink(c.font().blink()));
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
            if (VtKeys_.match(i, bufferEnd, k))
                keyDown(k);
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

    void AnsiRenderer::receivedSequence(tpp::Sequence::Kind, char const * buffer, char const * bufferEnd) {
        MARK_AS_UNUSED(buffer);
        MARK_AS_UNUSED(bufferEnd);
        NOT_IMPLEMENTED;
    }



} // namespace ui
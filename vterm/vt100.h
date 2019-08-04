#pragma once

#include <vector>

#include "terminal.h"

namespace vterm {


    /** Terminal understanding the ANSI escape sequences. 
     
        Should this be named differently? 
     */
    class VT100 : public Terminal {
    public:

        // log levels

		static constexpr char const* const SEQ = "VT100";
		static constexpr char const* const SEQ_UNKNOWN = "VT100_UNKNOWN";
		static constexpr char const* const SEQ_WONT_SUPPORT = "VT100_WONT_SUPPORT";


        VT100(int x, int y, int width, int height, PTY * pty, size_t ptyBufferSize = 10240);


    protected:
        void updateSize(int cols, int rows) override;

        void mouseDown(int col, int row, ui::MouseButton button, ui::Key modifiers) override;
        void mouseUp(int col, int row, ui::MouseButton button, ui::Key modifiers) override;
        void mouseWheel(int col, int row, int by, ui::Key modifiers) override;
        void mouseMove(int col, int row, ui::Key modifiers) override;
        void keyChar(helpers::Char c) override;
        void keyDown(ui::Key key) override;
        void keyUp(ui::Key key) override;

        size_t processInput(char * buffer, size_t bufferSize) override;


    private:

        class CSISequence;



        unsigned encodeMouseButton(ui::MouseButton btn, ui::Key modifiers);
        void sendMouseEvent(unsigned button, int col, int row, char end);

        /** Updates cursor position before modifying the cell it points to. 
         
            This is because the cursor position may temporarily be outside of the terminal size, such as when last character on the line is written and the cursor moves to next column, which is outside of the screen. This cannot be fixed when it happens because if the cursor is then moved by non cell-changing means, such as position change, por carriage return, no changes should be observed. 

            This function on the other hand is called before each observable change and makes sure that the cursor is positioned within the terminal. If the reposition requires changes, scrolls the terminal accordingly.
         */
        void updateCursorPosition();


        // TODO add comments and determine if it actually does what it is supposed to do, i.e. wrt CR and LF
		void invalidateLastCharPosition() {
			state_.lastCharCol = -1;
		}

		void markLastCharPosition() {
			if (state_.lastCharCol >= 0 && state_.lastCharCol < buffer_.width() &&
                state_.lastCharRow >= 0 && state_.lastCharRow < buffer_.height())
				buffer_.at(state_.lastCharCol, state_.lastCharRow) += ui::Attributes::EndOfLine();
		}

		void setLastCharPosition() {
			state_.lastCharCol = buffer_.cursor().col;
			state_.lastCharRow = buffer_.cursor().row;
		}


        enum class MouseMode {
            Off,
            Normal,
            ButtonEvent,
            All
        }; // VT100::MouseMode

        enum class MouseEncoding {
            Default,
            UTF8,
            SGR
        }; // VT100::MouseEncoding

        enum class CursorMode {
            Normal,
            Application
        }; // VT100::CursorMode

        struct State {
            /* Start of the scrolling region (inclusive row) */
            int scrollStart;
            /* End of the scrolling rehion (exclusive row) */
            int scrollEnd;
            /* Cell containing space and current fg, bg, decorations, etc. settings */
            ui::Cell cell;
            /* Location of the last valid character printed so that if followed by return, it can be set as line terminating. */
            int lastCharCol;
            int lastCharRow;

            State(int cols, int rows, ui::Color fg, ui::Color bg):
                scrollStart{0},
                scrollEnd{rows},
                cell{},
                lastCharCol{-1},
                lastCharRow{0} {
                cell << ' ' << ui::Foreground(fg) << ui::Background(bg) << ui::DecorationColor(fg);
                MARK_AS_UNUSED(cols);
            }

            void resize(int cols, int rows) {
                MARK_AS_UNUSED(cols);
                scrollStart = 0;
                scrollEnd = rows;
            }

        }; // VT100::State



        State state_;

        MouseMode mouseMode_;
        MouseEncoding mouseEncoding_;
        unsigned mouseLastButton_;
        /* Mouse buttons state */
        unsigned mouseButtonsDown_;

        CursorMode cursorMode_;



        std::string const * GetSequenceForKey(ui::Key key) {
            auto i = KeyMap_.find(key);
            if (i == KeyMap_.end())
                return nullptr;
            else
                return &(i->second);
        }

        static std::unordered_map<ui::Key, std::string> KeyMap_;

    }; // vterm::VT100


    /** Desrcibes parsed CSI sequence.

        The CSI sequence may have a first character and a last character which determine the kind of sequence and an arbitrary number of integer arguments.
     */
    class VT100::CSISequence {
    public:
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

        

    private:
        char firstByte_;
        char finalByte_;
        std::vector<std::pair<int, bool>> args_;




    }; // VT100::CSISequence


} // namespace vterm
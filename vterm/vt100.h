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




    }; 


} // namespace vterm
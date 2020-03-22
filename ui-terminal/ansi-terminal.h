#pragma once

#include <thread>

#include "helpers/log.h"
#include "helpers/process.h"

#include "ui2/renderer.h"
#include "ui2/widget.h"

#include "pty.h"

namespace ui2 {

    class AnsiTerminal : public Widget, public PTY::Client {
    public:

        // ========================================================================================

        /** 
         */
        class Buffer : public ui2::Buffer {
        public:
            Buffer(int width, int height):
                ui2::Buffer{width, height} {
                fill();
            }

            void fill() {
                for (int x = 0; x < width(); ++x)
                    for (int y = 0; y < height(); ++y) {
                        at(x, y).setCodepoint('0' + (x + y) % 10).setBg(Color{static_cast<unsigned char>(x), static_cast<unsigned char>(y), static_cast<unsigned char>(x + y)});
                    }
            }

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

        AnsiTerminal(int width = 0, int height = 0, int x = 0, int y = 0); 

        ~AnsiTerminal() override;

    protected:
        
        class CSISequence;
        class OSCSequence;

        class State {
        public:
            State(int cols, int rows):
                buffer{cols, rows} {
            }

            void resize(int cols, int rows) {
                buffer.resize(cols, rows);
                buffer.fill();
            }

            Buffer buffer;
            Cell cell;

            /** Current cursor position. */
            Point cursor;
            /** Start of the scrolling region (inclusive row) */
            int scrollStart;
            /** End of the scrolling rehion (exclusive row) */
            int scrollEnd;
            /** Determines whether inverse mode is active or not. */
            bool inverseMode;



        protected:

            std::vector<Point> cursorStack_;

        }; // AnsiTerminal::State

        /** \name Rendering & user input
         */
        //@{

        void paint(Canvas & canvas) override;

        void setRect(Rect const & value) override;

        //@}



        /** \name PTY Interface 
         */
        //@{

        /** Parses the given input. Returns the number of bytes actually parsed. 
         */
        size_t processInput(char const * buffer, char const * bufferEnd) override;

        void ptyTerminated(ExitCode exitCode) override {
            Renderer::SendEvent([this, exitCode](){ });
        }
        //@}

        /** \name Input Parsing
         
            Input processing can happen in non-UI thread.
         */
        //@{

        void parseCodepoint(char32_t codepoint);

        void parseNotification();

        size_t parseEscapeSequence(char const * buffer, char const * bufferEnd);


        //@}

        /** The terminal state. 
         */
        State state_;




        static std::unordered_map<Key, std::string> KeyMap_;
        static char32_t LineDrawingChars_[15];

    }; // ui2::AnsiTerminal

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

    }; // ui::AnsiTerminal::CSISequence

    // ============================================================================================

    class AnsiTerminal::OSCSequence {
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

    }; // ui::AnsiTerminal::OSCSequence

} // namespace ui
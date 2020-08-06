#pragma once

#include <string>

namespace ui {

    class OSCSequence {
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
        static OSCSequence Parse(char const * & buffer, char const * end);

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

    }; // ui::OSCSequence


} // namespace ui
#pragma once

#include <string>
#include <vector>

#include <helpers/char.h>

namespace tpp {


    
    class Encoder {
    public:
        Encoder() = default;

        void encode(char const * data, size_t length) {
            clear();
            while (length-- > 0) {
                switch (*data) {
                    case helpers::Char::BEL:
                    case helpers::Char::ESC:
                    case '`':
                        buffer_.push_back('`');
                        buffer_.push_back(helpers::ToHexDigit(*data >> 4));
                        buffer_.push_back(helpers::ToHexDigit(*data & 0xf));
                        break;
                    default:
                        buffer_.push_back(*data);
                }
                ++data;
            }
        }

        void append(char what) {
            buffer_.push_back(what);
        }

        char const * buffer() const {
            return buffer_.data();
        }

        size_t size() const {
            return buffer_.size();
        }

        void clear() {
            buffer_.clear();
        }

    protected:

        std::vector<char> buffer_;

    }; // tpp::Encoder

} // namespace tpp


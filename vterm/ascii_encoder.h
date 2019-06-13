#pragma once
#include <sstream>
#include <string>

#include "helpers/strings.h"

#include "vt100.h"

namespace vterm {

    class ASCIIEncoder {
    public:
        class VT100 : public vterm::VT100 {
        public:
            VT100(PTY * pty, Palette const & palette, unsigned defaultFg, unsigned defaultBg):
                vterm::VT100(pty, palette, defaultFg, defaultBg) {
            }

        protected:
            size_t dataReceived(char * buffer, size_t size) override {
                //return vterm::VT100::dataReceived(buffer, size);
                size_t decodedSize;
                size_t undecoded = size - Decode(buffer, size, decodedSize);
                size_t processed = vterm::VT100::dataReceived(buffer, decodedSize);
                // to preserve the semantics, the unprocessed data must be copied at the very end, but before any undecoded data left
                size_t unprocessed = decodedSize - processed;
                if (unprocessed > 0) 
                    memcpy(buffer + size - unprocessed - undecoded, buffer + processed, decodedSize - processed);
                return size - unprocessed - undecoded;
            }

            size_t sendData(char const * buffer, size_t size) override {
                std::stringstream ss;
                Encode(ss, buffer, size);
                std::string str = ss.str();
                return vterm::VT100::sendData(str.c_str(), str.size());
            }
        }; // ASCIIEncoder::VT100

        static void Encode(std::ostream & output, char const * buffer, size_t bufferSize) {
            while (bufferSize-- > 0) {
                unsigned char c = static_cast<unsigned char>(*(buffer++));
                if (c == '`') {
                    output << "``";
                } else if (c >= ' ' && c <= '~') {
                    output << c;
                } else if (c < ' ') {
                    output << '`' << static_cast<char>(c + '@');
                } else {
                    output << '`' << helpers::ToHexDigit(c >> 4) << helpers::ToHexDigit(c & 0xf);
                }
            }
        }

        /** Decodes given buffer.

         */
        static size_t Decode(char * buffer, size_t bufferSize, size_t & decodedSize) {
            decodedSize = 0;
            size_t i = 0;
            while (i < bufferSize) {
                if (buffer[i] != '`') {
                    buffer[decodedSize++] = buffer[i];
                    ++i;
                } else {
                    // if there is not enough 
                    if (i + 1 >= bufferSize)
                        break;
                    char c = buffer[i + 1];
                    if (c == '`') {
                        buffer[decodedSize++] = '`';
                        i += 2;
                    } else if (c >= '@' && c <= '`') {
                        buffer[decodedSize++] = static_cast<char>(c - '@');
                        i += 2;
                    } else {
                        if (i + 2 >= bufferSize)
                            break;
                        buffer[decodedSize++] = static_cast<char>((helpers::HexCharToNumber(buffer[i + 1]) << 4) + helpers::HexCharToNumber(buffer[i + 2]));
                        i += 3;
                    }
                }
            }
            return i;
        }
    };
} // namespace vterm
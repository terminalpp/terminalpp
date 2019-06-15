#pragma once
#include <sstream>
#include <string>

#include "helpers/strings.h"
#include "helpers/log.h"

#include "vt100.h"

namespace vterm {

    class ASCIIEncoder {
    public:
		class CommandHandler {
		public:
			virtual void resize(unsigned cols, unsigned rows) = 0;
		};

        class VT100 : public vterm::VT100 {
        public:
            VT100(PTY * pty, Palette const & palette, unsigned defaultFg, unsigned defaultBg):
                vterm::VT100(pty, palette, defaultFg, defaultBg) {
            }

        protected:
            size_t dataReceived(char * buffer, size_t size) override {
				// size of data to be decoded first (we may have decoded, but unprocessed data at the beginning of the buffer from last time)
				size_t sizeToDecode = size - alreadyDecoded_;
                size_t decodedSize;
                size_t undecoded = sizeToDecode - Decode(buffer + alreadyDecoded_, sizeToDecode, decodedSize);
				// the data to process is size of already decoded data and the data decoded in this step
                size_t processed = vterm::VT100::dataReceived(buffer, decodedSize + alreadyDecoded_);
				// we must make sure that the end of the buffer corresponds to first unprocessed and then undecoded data to preserve the semantics of the underlying mechanism. Any undecoded data is already there, so we only need to deal with unprocessed
				size_t unprocessed = decodedSize + alreadyDecoded_ - processed;
				if (unprocessed > 0) {
					memcpy(buffer + size - undecoded - unprocessed, buffer + processed, unprocessed);
				}
				alreadyDecoded_ = unprocessed;
				return size - unprocessed - undecoded;
            }

            size_t sendData(char const * buffer, size_t size) override {
                std::stringstream ss;
                Encode(ss, buffer, size);
                std::string str = ss.str();
                return vterm::VT100::sendData(str.c_str(), str.size());
            }

		private:
			size_t alreadyDecoded_ = 0;
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

		static size_t Decode(char* buffer, size_t bufferSize, size_t& decodedSize, CommandHandler* handler = nullptr) {
#define DECODE(WHAT) buffer[decodedSize++] = WHAT
#define NEXT if (++i == bufferSize) return processed
			decodedSize = 0;
			size_t processed = 0;
			size_t i = 0;
			while (true) {
				processed = i;
				if (processed == bufferSize)
					return processed;
				// if current character is not a backtick, copy the character as it is
				if (buffer[i] != '`') {
					DECODE(buffer[i]);
				// otherwise move to the next character and decide what are we dealing with
				} else {
					NEXT;
					if (buffer[i] == '`') {
						DECODE('`');
					} else if (buffer[i] == 'x') {
						NEXT;
						switch (buffer[i]) {
							// resize `xrCOLS:ROWS;
						    case 'r': {
								NEXT;
								unsigned cols;
								unsigned rows;
								if (!ParseNumber(buffer, bufferSize, i, cols))
									return processed;
								ASSERT(buffer[i] == ':') << "expected :, found " << buffer[i];
								NEXT;
								if (!ParseNumber(buffer, bufferSize, i, rows))
									return processed;
								ASSERT(buffer[i] == ';') << "expected ;";
								if (handler != nullptr)
									handler->resize(cols, rows);
								break;
							}
							default:
								UNREACHABLE;
						}
					} else if (buffer[i] >= '@' && buffer[i] < '`') {
						DECODE(static_cast<char>(buffer[i] - '@'));
					} else {
						unsigned value = helpers::HexCharToNumber(buffer[i]) << 4;
						NEXT;
						value += helpers::HexCharToNumber(buffer[i]);
						DECODE(static_cast<char>(value));
					}
				}
				++i;
			}
#undef DECODE
#undef NEXT
		}

	private:

		static bool ParseNumber(char* buffer, size_t bufferSize, size_t& i, unsigned & value) {
			value = 0;
			while (helpers::IsDecimalDigit(buffer[i])) {
				value = value * 10 + (buffer[i] - '0');
				if (++i == bufferSize)
					return false;
			}
			return true;
		}

    };
} // namespace vterm
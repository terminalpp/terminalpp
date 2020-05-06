#include "helpers/char.h"

#include "terminal_client.h"

namespace tpp {

    using Char = helpers::Char;


    void TerminalClient::start() {
        reader_ = std::thread{[this](){
            readerThread();
        }};
    }

    void TerminalClient::readerThread() {
        // create the buffer, and read while we can 
        size_t bufferSize = DEFAULT_BUFFER_SIZE;
        char * buffer = new char[bufferSize];
        char * writeStart = buffer;
        bool success = false;
        while (true) {
            size_t bytesRead = pty_.receive(writeStart, bufferSize - (writeStart - buffer), success);
            if (!success)
                break;
            bytesRead += (writeStart - buffer);
            char * unprocessed = parseTerminalInput(buffer, buffer + bytesRead);
            // TDOO if buffer is full, grow it up to some size
            // copy the unprocessed part of the buffer from the end of it to its beginning and set writing of new data after it
            if (unprocessed < buffer + bytesRead) {
                size_t unprocessedBytes = (buffer + bytesRead) - unprocessed;
                memmove(buffer, unprocessed, unprocessedBytes);
                writeStart = buffer + unprocessedBytes;
            } else {
                writeStart = buffer;
            }
        }
        // if there is any unprocessed input, try processing it (it could have been leftovers from partial tpp sequence that would make sense for the parser)
        if (writeStart != buffer)
            writeStart -= receive(buffer, writeStart);
        // the terminal has been closed when the input pty eofs
        inputEof(buffer, writeStart);
        // and delete the buffer
        delete [] buffer;
    }

    char * TerminalClient::parseTerminalInput(char * buffer, char const * bufferEnd) {
        while (buffer != bufferEnd) {
            char * tppStart = buffer;
            std::pair<char*, char*> tppRange;
            while (true) {
                tppStart = findTppStartPrefix(tppStart, bufferEnd);
                tppRange = findTppRange(tppStart, bufferEnd);
                // possibly valid tpp sequence or end of buffer found
                if (tppRange.first != nullptr)
                    break;
                // invalid sequence found, move past it and try again
                ++tppStart;
            }
            // process the normal input before the sequence 
            size_t processed = 0;
            if (tppStart != buffer)
                processed = receive(buffer, tppStart);
            size_t unprocessed = tppStart - buffer - processed;
            // if the sequence was valid, process it, then copy any unprocessed normal input preceding it towards its end and move buffer 
            if (tppRange.second != bufferEnd) {
                receive(tppRange.first, tppRange.second);
                ++tppRange.second;
                if (unprocessed > 0) {
                    memmove(tppRange.second, buffer + processed, unprocessed );
                    buffer = tppRange.second + unprocessed;
                } else {
                    buffer = tppRange.second;
                }
            // otherwise move any unprocessed data *before* the tppStart and return as we need more data
            } else {
                if (unprocessed > 0) {
                    tppStart -= unprocessed;
                    memmove(tppStart, buffer - processed, unprocessed);
                }
                return tppStart;
            }
        }
        return buffer;
    }

    char * TerminalClient::findTppStartPrefix(char * buffer, char const * bufferEnd) {
        for (; buffer < bufferEnd; ++buffer) {
            if (buffer[0] == '\033') {
                if (buffer + 1 < bufferEnd) {
                    if (buffer[1] == 'P')
                        if (buffer + 2 < bufferEnd || buffer[2] == '+')
                            return buffer; // found or prefix
                } else {
                    return buffer; // prefix
                }
            }
        }
        return buffer; // not found, buffer is bufferEnd
    }

    std::pair<char *, char*> TerminalClient::findTppRange(char * tppStart, char const * bufferEnd) {
        // if we don't see entire tpp start, don't change the tpp start
        tppStart += 3;
        if (tppStart >= bufferEnd)
            return std::pair<char *, char *>{const_cast<char*>(bufferEnd), const_cast<char*>(bufferEnd)};
        // time to parse the message size and then try to find the end of it
        size_t size = 0;
        unsigned digit = 0;
        while (tppStart < bufferEnd) {
            if (*tppStart == ';') {
                ++tppStart;
                char * tppEnd = tppStart + size;
                if (tppEnd >= bufferEnd)
                    return std::pair<char *, char *>{const_cast<char*>(bufferEnd), const_cast<char*>(bufferEnd)};
                if (*tppEnd != Char::BEL) {
                    LOG() << "Mismatched t++ sequence terminator";
                    return std::pair<char*, char*>{nullptr, nullptr};
                }
                return std::make_pair(tppStart, tppEnd);
            } else if (!Char::IsHexadecimalDigit(*tppStart, digit)) {
                LOG() << "Invalid characters it tpp sequence size";
                return std::pair<char*, char*>{nullptr, nullptr};
            }
            size = (size * 16) + digit;
            ++tppStart;
        }
        // incomplete sequence length
        return std::pair<char *, char *>{const_cast<char*>(bufferEnd), const_cast<char*>(bufferEnd)};
    }

} // namespace tpp
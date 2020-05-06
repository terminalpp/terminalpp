#include "terminal_pty.h"

namespace tpp {

#if (defined ARCH_UNIX)

    void StdPTY::send(char const * buffer, size_t numBytes) {
        if (insideTmux_) {
            // we have to properly escape the buffer
            size_t start = 0;
            size_t end = 0;
            while (end < numBytes) {
                if (buffer[end] == '\033') {
                    if (start != end)
                        ::write(out_, buffer + start, end - start);
                    ::write(out_, "\033\033", 2);
                    start = ++end;
                } else {
                    ++end;
                }
            }
            if (start != end)
                ::write(out_, buffer + start, end - start);
        } else {
            ::write(out_, buffer, numBytes);
        }
    }

    void StdPTY::send(Sequence const & seq) {
        if (insideTmux_)
            ::write(out_, "\033Ptmux;", 7);
        TerminalPTY::send(seq);
        if (insideTmux_)
            ::write(out_, "033\\", 2);
    }

    size_t StdPTY::receive(char * buffer, size_t bufferSize, bool & success) {
        while (true) {
            int cnt = 0;
            cnt = ::read(in_, (void*)buffer, bufferSize);
            if (cnt == -1) {
                if (errno == EINTR || errno == EAGAIN)
                    continue;
                success = false;
                return 0;
            } else {
                success = true;
                return static_cast<size_t>(cnt);
            }
        }
    }

#endif // ARCH_UNIX

} // namespace tpp
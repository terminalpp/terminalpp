#pragma once
#if (defined ARCH_LINUX)

#include <termios.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

namespace tpp {

    class RawMode {
    public:
        RawMode() {
            tcgetattr(STDIN_FILENO, & backup_);
            termios raw = backup_;
            raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
            raw.c_oflag &= ~(OPOST);
            raw.c_cflag |= (CS8);
            raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
            tcsetattr(STDIN_FILENO, TCSAFLUSH, & raw);
        }

        ~RawMode() {
            tcsetattr(STDIN_FILENO, TCSAFLUSH, & backup_);
        }

    private:
        termios backup_;
    };


    class NonBlockingInput {
    public:
        NonBlockingInput(int fileno = STDIN_FILENO):
            fileno_(fileno) {
            fcntl(fileno_, F_SETFL, fcntl(STDIN_FILENO, F_GETFL) | O_NONBLOCK);
        }

        ~NonBlockingInput() {
            fcntl(fileno_, F_SETFL, fcntl(STDIN_FILENO, F_GETFL) & ~O_NONBLOCK);
        }

    private:
        int const fileno_;
    };

} // namespace tpp



#endif
#if (defined ARCH_LINUX)
#include <termios.h>
#include <unistd.h>
#endif

#include <thread>

#include "helpers/helpers.h"

#include "raw_mode.h"
#include "sequence.h"
#include "commands.h"

namespace tpp {

    namespace {

        constexpr char const * TPP_ESCAPE = "\033]+";
        constexpr char const * TPP_END = "\007";

    } // anonymous namespace

#if (defined ARCH_LINUX)
    response::Capabilities GetCapabilities(size_t timeout) {
        {
            std::string x(STR(TPP_ESCAPE << Sequence::Capabilities << TPP_END));
            ::write(STDOUT_FILENO, x.c_str(), x.size());
        }
        return response::Capabilities(Sequence::WaitAndRead(STDIN_FILENO, timeout));
    }

    int NewFile(std::string const & filename, size_t size, size_t timeout) {
        {
            std::string x(STR(TPP_ESCAPE << Sequence::NewFile << ";" << size << ";" << filename << TPP_END));
            ::write(STDOUT_FILENO, x.c_str(), x.size());
        }
        response::NewFile x{Sequence::WaitAndRead(STDIN_FILENO, timeout)};
        return x.fileId();
    }

    void Send(int fileId, char const * data, size_t numBytes, Encoder & enc) {
        std::string x(STR(TPP_ESCAPE << Sequence::Send << ";" << fileId));
        ::write(STDOUT_FILENO, x.c_str(), x.size());
        enc.encode(data, numBytes);
        enc.append(helpers::Char::BEL);
        ::write(STDOUT_FILENO, enc.buffer(), enc.size());
    }

    void Open(int fileId) {
        std::string x(STR(TPP_ESCAPE << Sequence::Open << ";" << fileId << TPP_END));
        ::write(STDOUT_FILENO, x.c_str(), x.size());
    }
#endif



}
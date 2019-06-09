#include <cstdlib>
#include <unistd.h>
#include <iostream>
#include <thread>

#include "helpers/helpers.h"

#include "vterm/local_pty.h"

size_t constexpr BUFFER_SIZE = 10240;

helpers::Command GetCommand(int argc, char * argv[]) {
    if (argc < 2)
        THROW(helpers::Exception()) << "Invalid number of arguments - at least the command to execute must be specified";
    std::vector<std::string> args;
    for (int i = 2; i < argc; ++i)
        args.push_back(argv[i]);
    return helpers::Command(argv[1], args);
}

void EncodeOutput(char * buffer, size_t numBytes) {
    for (size_t i = 0; i < numBytes; ++i) 
        std::cout << buffer[i];
    std::cout << std::flush;
}

void EncodeInput(char * buffer, size_t & numBytes) {
}

int main(int argc, char * argv[]) {
    try {
        // get the command
        helpers::Command cmd = GetCommand(argc, argv);
        // TODO we need the stdio in raw mode (!!)

        // create the pty
        vterm::LocalPTY pty(cmd);
        // start the pty reader and encoder thread
        std::thread outputEncoder([&]() {
            char * buffer = new char[BUFFER_SIZE];
            size_t numBytes;
            while (true) {
                numBytes = pty.receiveData(buffer, BUFFER_SIZE);
                // if nothing was read, the process has terminated and so should we
                if (numBytes == 0)
                    break;
                EncodeOutput(buffer, numBytes);
            }
            delete [] buffer;
        });
        // start the cin reader and encoder thread
        std::thread inputEncoder([&]() {
            char * buffer = new char[BUFFER_SIZE];
            size_t numBytes;
            while (true) {
                numBytes = read(STDIN_FILENO, (void *) buffer, BUFFER_SIZE);
                if (numBytes == 0)
                    break;
                EncodeInput(buffer, numBytes);
                //if (pty.sendData(buffer, numBytes) == 0)
                //    break;
            }
            delete [] buffer;
        });
        inputEncoder.detach();
        helpers::ExitCode ec = pty.waitFor();
        // close stdin so that the inputEncoder will stop
        //close(STDIN_FILENO);
        outputEncoder.join();
        //inputEncoder.join();
        return ec;
    } catch (helpers::Exception const & e) {
        std::cerr << "asciienc error: " << e << std::endl;
    }
    return EXIT_FAILURE;

}
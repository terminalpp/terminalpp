#include <cstdlib>
#include <iostream>
#include <fstream>

#include "helpers/helpers.h"

#include "tpp-lib/terminal.h"


void ProgressBar(size_t value, size_t max, size_t barWidth = 40) {
    size_t pct = value * 100 / max;
    size_t barX = barWidth * pct / 100;
    std::cout <<  "[\033[32m";
    for (size_t i = 0; i < barX; ++i)
        std::cout <<  '#';
    std::cout << "\033[0m";
    for (size_t i = barX; i < barWidth; ++i)
        std::cout <<  ' ';
    std::cout <<  "] " << pct << "% ";
}



/** Takes the given file and transfers it via the standard output. 
 */
int TransferFile(tpp::Terminal & t, std::string const & filename, size_t messageLength) {
    std::ifstream f(filename);
    if (!f.good()) {
        std::cerr << "Unable to open file " << filename << "\r";
        return EXIT_FAILURE;
    }
    f.seekg(0, std::ios_base::end);
    size_t numBytes = f.tellg();
    f.seekg(0, std::ios_base::beg);
    std::cout << "Transferring file " << filename << ", " << numBytes << " bytes\r";

    // obtain the file descriptor
    int fileId = t.newFile(filename, numBytes);
    if (fileId < 0) {
        std::cerr << "Unable to open file for the terminal++ session.\r";
        return EXIT_FAILURE;
    }
    
    // read the file
    std::cout << "Transmitting...\r";

    //std::cout << "\033[K" << std::flush; // clear the line
    std::cout << "\r";
    char * buffer = new char[messageLength];
    size_t sentBytes = 0;
    while (sentBytes < numBytes) {
        f.read(buffer, messageLength);
        size_t chunkSize = f.gcount();
        t.transmit(fileId, buffer, chunkSize);
        sentBytes += chunkSize;
        ProgressBar(sentBytes, numBytes);
        std::cout <<  "    \r" << std::flush;
    }

    // instruct the terminal to open the given file descriptor
    t.openFile(fileId);

    return EXIT_SUCCESS;
}

int main(int argc, char * argv[]) {
    tpp::StdTerminal t;
    int result = EXIT_FAILURE;
    try {
        if (argc != 2) {
            std::cerr << "Filename not specified.\r";
        } else {
            tpp::response::Capabilities cap{t.getCapabilities()};
            if (!cap.valid()) {
                std::cerr << "Unable to open the file - not attached to terminal++\r";
            } else {
                std::cerr << "terminal++: version " << cap.version() << " detected\r";
                result = TransferFile(t, argv[1], 1024);
            }
        }
    } catch (std::exception const & e) {
        std::cerr << "Error: " << e.what() << "\r\n";
    } catch (...) {
        std::cerr << "Unknown Error \r\n";
    }
    std::cout << "\r\n";
    return result;
}
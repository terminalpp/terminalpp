#include <cstdlib>
#include <iostream>
#include <fstream>

#include "helpers/helpers.h"

#include "tpp-lib/raw_mode.h"
#include "tpp-lib/commands.h"

#include "tpp-lib/sequence.h"
#include "tpp-lib/encoder.h"


/** Takes the given file and transfers it via the standard output. 
 */
int TransferFile(std::string const & filename, size_t messageLength) {
    std::ifstream f(filename);
    if (!f.good()) {
        std::cerr << "Unable to open file " << filename << "\r\n";
        return EXIT_FAILURE;
    }
    f.seekg(0, std::ios_base::end);
    size_t numBytes = f.tellg();
    f.seekg(0, std::ios_base::beg);
    std::cout << "Transferring file " << filename << ", " << numBytes << " bytes\r\n";

    // obtain the file descriptor
    int fileId = tpp::NewFile(filename, numBytes);
    if (fileId < 0) {
        std::cerr << "Unable to open file for the terminal++ session.\r\n";
        return EXIT_FAILURE;
    }
    
    // read the file

    tpp::Encoder enc{};    
    char * buffer = new char[messageLength];
    while (numBytes > 0) {
        f.read(buffer, messageLength);
        size_t chunkSize = f.gcount();
        tpp::Send(fileId, buffer, chunkSize);
        numBytes -= chunkSize;
    }

    // instruct the terminal to open the given file descriptor
    tpp::OpenFile(fileId);

    return EXIT_SUCCESS;
}

int main(int argc, char * argv[]) {
    tpp::RawMode rm{};
    if (argc != 2) {
        std::cerr << "Filename not specified.\r\n";
    } else {
        tpp::response::Capabilities cap{tpp::GetCapabilities()};
        if (!cap.valid()) {
            std::cerr << "Unable to open the file - not attached to terminal++\r\n";
        } else {
            std::cerr << "terminal++: version " << cap.version() << " detected\r\n";
            return TransferFile(argv[1], 1024);
        }
    }
    return EXIT_FAILURE;
}
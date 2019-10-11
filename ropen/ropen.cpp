#include <cstdlib>
#include <iostream>
#include <fstream>

#include "helpers/helpers.h"

#include "tpp-lib/sequences.h"
#include "tpp-lib/encoder.h"


/** Takes the given file and transfers it via the standard output. 
 */
void TransferFile(std::string const & filename, size_t messageLength) {
    std::ifstream f(filename);
    f.seekg(0, std::ios_base::end);
    size_t numBytes = f.tellg();
    f.seekg(0, std::ios_base::beg);
    std::cout << "Transferring file " << filename << ", " << numBytes << " bytes" << std::endl;

    // obtain the file descriptor
    int fileId = 0;
    {
        std::string x = STR("\033]+" << tpp::OSCSequence::NewFile << ";" << numBytes << ";" << filename << "\007");
        std::cout << x << std::flush;
    }

    
    // read the file

    tpp::Encoder enc(STR("\033]+" << tpp::OSCSequence::Send << ";" << fileId << ";"));    
    char * buffer = new char[messageLength];
    while (numBytes > 0) {
        f.read(buffer, messageLength);
        size_t chunkSize = f.gcount();
        enc.encode(buffer, chunkSize);
        enc.append(helpers::Char::BEL);
        std::cout.write(enc.buffer(), enc.size());
        numBytes -= chunkSize;
    }

    // instruct the terminal to open the given file descriptor
    {
        std::string x = STR("\033]+" << tpp::OSCSequence::Open << ";" << fileId << "\007");
        std::cout << x << std::flush;
    }
}





int main(int argc, char * argv[]) {

    TransferFile(argv[1], 1024);

    return EXIT_SUCCESS;
}
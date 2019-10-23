#include <cstdlib>
#include <iostream>
#include <fstream>
#include <memory>

#include "helpers/helpers.h"

#include "tpp-lib/terminal.h"

namespace tpp {

    class RemoteOpen {
    public:
        static constexpr size_t GOOD_PACKET_LIMIT = 128;
        static constexpr size_t BAD_PACKET_LIMIT = 8;

        static void Open(Terminal & terminal, std::string const & localFile) {
            RemoteOpen o{terminal, localFile};
            o.checkCapabilities();
            terminal.setTimeout(1000);
            o.initialize();
            o.transfer();
            o.open();
        }

    private:
        RemoteOpen(Terminal & t, std::string const & localFile):
            t_(t),
            localFile_(localFile),
            packetSize_(1024),
            packets_{0},
            packetLimit_{GOOD_PACKET_LIMIT},
            totalBytes_{0},
            sentBytes_{0},
            barWidth_{40} {
        }

        void checkCapabilities() {
            tpp::response::Capabilities cap{t_.getCapabilities()};
            if (!cap.valid()) 
                THROW(helpers::IOError()) << "Unable to connect to terminal++";
        }

        /** Initializes the transmission and obtains the file id. 
         */    
        void initialize() {
            s_.open(localFile_);
            if (! s_.good())
                THROW(helpers::IOError()) << "Unable to open local file " << localFile_;
            s_.seekg(0, std::ios_base::end);
            totalBytes_ = s_.tellg();
            s_.seekg(0, std::ios_base::beg);

            fileId_ = t_.newFile(localFile_, totalBytes_);
        }

        void transfer() {
            std::unique_ptr<char> buffer{new char[packetSize_]};
            sentBytes_ = 0;
            packets_ = 0;
            while (sentBytes_ != totalBytes_) {
                s_.read(buffer.get(), packetSize_);
                size_t pSize = s_.gcount();
                t_.transmit(fileId_, sentBytes_, buffer.get(), pSize);
                sentBytes_ += pSize;
                checkTransferStatus();
                progressBar();
            }
        }

        void open() {
            t_.openFile(fileId_);
        }

        void checkTransferStatus() {
            if (packets_++ == packetLimit_ || sentBytes_ == totalBytes_) {
                // TODO how many retrials we want
                for (size_t i = 0; i < 5; ++i) {
                    try {
                        size_t transferred = t_.transferStatus(fileId_);
                        if (transferred != sentBytes_) {
                            // decrease packet limit
                            if (packetLimit_ > 8)
                                packetLimit_ >>= 1;
                            sentBytes_ = transferred;
                            s_.clear();
                            s_.seekg(sentBytes_);
                        } else if (packetLimit_ < GOOD_PACKET_LIMIT) {
                            packetLimit_ <<= 2;
                        }
                        packets_ = 0;
                        return;
                    } catch (helpers::TimeoutError const &) {
                        continue;
                    } catch (...) {
                        throw;
                    }
                }
            }
        }

        void progressBar() {
            size_t pct = sentBytes_ * 100 / totalBytes_;
            size_t barX = barWidth_ * pct / 100;
            if (packetLimit_ >= GOOD_PACKET_LIMIT)
                std::cout << "[\033[32m";
            else if (packetLimit_ >= BAD_PACKET_LIMIT)
                std::cout << "[\033[33m";
            else 
                std::cout << "[\033[91m";
            for (size_t i = 0; i < barX; ++i)
                std::cout <<  '#';
            std::cout << "\033[0m";
            for (size_t i = barX; i < barWidth_; ++i)
                std::cout <<  ' ';
            std::cout <<  "] " << pct << "% \r" << std::flush;
        }

        Terminal & t_;
        std::string const & localFile_;
        std::ifstream s_;
        int fileId_;
        size_t packetSize_;
        size_t packets_;
        size_t packetLimit_;
        size_t totalBytes_;
        size_t sentBytes_;

        size_t barWidth_;

    };

}

#ifdef FOO

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
int TransferFile(tpp::Terminal & t, std::string const & filename, size_t messageLength, size_t packetLimit) {
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

    char * buffer = new char[messageLength];
    size_t sentBytes = 0;
    size_t packets = 0;
    size_t currentPacketLimit = 256;
    while (true) {
        if (packets++ == currentPacketLimit || sentBytes == numBytes) {
            size_t transmitted = t.transferStatus(fileId);
            if (transmitted != sentBytes) {
                //std::cout << "Error, restarting from " << transmitted << "\r\n";
                sentBytes = transmitted;
                f.clear();
                f.seekg(sentBytes);
            } else {
                //std::cout << "OK, transmitted " << transmitted << " out of " << numBytes << "\r\n";
            }
            packets = 0;
        }
        if (sentBytes == numBytes)
            break;
        f.read(buffer, messageLength);
        size_t chunkSize = f.gcount();
        if (rand() % 100 > 96)
            t.transmit(fileId, sentBytes + 10, buffer, chunkSize);
        else
            t.transmit(fileId, sentBytes, buffer, chunkSize);
        sentBytes += chunkSize;
        ProgressBar(sentBytes, numBytes);
        std::cout <<  "    \r" << std::flush;
    }

    // instruct the terminal to open the given file descriptor
    //  t.openFile(fileId);

    return EXIT_SUCCESS;
}


#endif

int main(int argc, char * argv[]) {
    try {
        tpp::StdTerminal t;
        tpp::RemoteOpen::Open(t, argv[1]);
        std::cout << "\n";
        return EXIT_SUCCESS;
    } catch (std::exception const & e) {
        std::cout << "\r\n Error: " << e.what() << "\r\n";
    } catch (...) {
        std::cout << "\r\n Unspecified errror.\r\n";
    }
    return EXIT_FAILURE;
    /*
    tpp::StdTerminal t;
    int result = EXIT_FAILURE;
    try {
        if (argc != 2) {
            std::cerr << "Filename not specified.\r";
        } else {
            tpp::response::Capabilities cap{t.getCapabilities()};
            if (!cap.valid()) {
                std::cerr << "Unable to open the file - not attached to terminal\r";
            } else {
                std::cerr << "terminal: version " << cap.version() << " detected\r";
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
    */
}
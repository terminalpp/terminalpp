#include <cstdlib>
#include <iostream>
#include <fstream>
#include <memory>
#include <filesystem>

#include "helpers/helpers.h"
#include "helpers/version.h"
#include "helpers/filesystem.h"
#include "helpers/json_config.h"

#include "tpp-lib/local_pty.h"
#include "tpp-lib/terminal_client.h"

#include "stamp.h"

namespace tpp {

    class Config : public JSONConfig::CmdArgsRoot {
    public:
        CONFIG_PROPERTY(
            timeout,
            "Timeout of the connection to terminal++ (in ms)",
            JSON{1000},
            unsigned
        );
        CONFIG_PROPERTY(
            adaptiveSpeed,
            "Adaptive speed",
            JSON{true},
            bool
        );
        CONFIG_PROPERTY(
            packetSize,
            "Size of single packet of data",
            JSON{1024},
            unsigned
        );
        CONFIG_PROPERTY(
            packetLimit,
            "Number of packets that can be sent without waiting for acknowledgement",
            JSON{32},
            unsigned
        );
        CONFIG_PROPERTY(
            filename, 
            "Local file to be opened on the remote machine",
            JSON{""},
            std::string
        );
        CONFIG_PROPERTY(
            verbose,
            "Verbose output",
            JSON{false},
            bool
        );

        static Config & Instance() {
            static Config singleton{};
            return singleton;
        }

        static Config & Setup(int argc, char * argv[]) {
            Config & config = Instance();
            //config.fillDefaultValues();
            config.parseCommandLine(argc, argv);
            if (! config.filename.updated())
                THROW(ArgumentError()) << "Input file must be specified";
            return config;
        }

    private:

        Config() {
            addArgument(timeout, {"--timeout", "-t"});
            addArgument(packetSize, {"--packet-size"});
            addArgument(verbose, {"--verbose", "-v"});
            addArgument(adaptiveSpeed, {"--adaptive"});
            addArgument(filename, {"--file", "-f"});
            setDefaultArgument(filename);
        }

    }; // tpp::Config

    class RemoteOpen {
    public:

        static constexpr size_t MIN_PACKET_LIMIT = 8;

        static void Transfer(TerminalClient::Sync & t, std::string const & filename) {
            RemoteOpen r{t, Config::Instance()};
            r.openLocalFile(filename);
            r.transfer();
            r.view();
        }

    private:

        RemoteOpen(TerminalClient::Sync & t, Config const & config):
            t_{t},
            adaptiveSpeed_{config.adaptiveSpeed()},
            packetSize_{config.packetSize()},
            packetLimit_{config.packetLimit()},
            initialPacketLimit_{packetLimit_} {
            // register sigint handler so that we clear the terminal client properly
            struct sigaction sa;
            sigemptyset(&sa.sa_mask);
            sa.sa_handler = SIGINT_handler;
            sa.sa_flags = 0;        
            OSCHECK(sigaction(SIGINT, &sa, nullptr) == 0);        
            // verify the t++ capabilities of the terminal
            Sequence::Capabilities capabilities{t_.getCapabilities()};
            if (capabilities.version() != 1)
                THROW(Exception()) << "Incompatible t++ version " << capabilities.version() << " (required version 1)";
        }

        void openLocalFile(std::string const & filename) {
            try {
                std::string remoteHost = GetHostname();
                LOG(Log::Verbose) << "Remote host: " << remoteHost;
                std::string remoteFile = std::filesystem::canonical(filename);
                LOG(Log::Verbose) << "Remote file canonical path: " << remoteFile;
                f_.open(remoteFile);
                if (!f_.good())
                    throw false;
                f_.seekg(0, std::ios_base::end);
                size_ = f_.tellg();
                LOG(Log::Verbose) << "    size: " << size_;
                streamId_ = t_.openFileTransfer(remoteHost, remoteFile, size_);
                LOG(Log::Verbose) << "Assigned stream id: " << streamId_;
            } catch (...) {
                THROW(IOError()) << "Unable to open file " << filename;
            }
        }

        void transfer() {
            std::unique_ptr<char> buffer{new char[packetSize_]};
            LOG(Log::Verbose) << "Transferring, packet limit: " << packetLimit_;
            f_.seekg(0, std::ios_base::beg);
            sent_ = 0;
            size_t packets = 0;
            while (sent_ != size_) {
                if (Interrupted_)
                    THROW(Exception()) << "Interrupted";
                f_.read(buffer.get(), packetSize_);
                size_t pSize = f_.gcount();
                // TODO this is a memory copy, can be done more effectively by having a non-ownership version of the Data message. 
                Sequence::Data d{streamId_, sent_, buffer.get(), buffer.get() + pSize};
                t_.send(d);
                sent_ += pSize;
                if (++packets == packetLimit_ || sent_ == size_) {
                    packets = 0;
                    checkTransferStatus();
                    progressBar();
                }
            }
        }

        void checkTransferStatus() {
            Sequence::TransferStatus ts{t_.getTransferStatus(streamId_)};
            if (ts.received() == sent_) {
                if (adaptiveSpeed_ && packetLimit_ < initialPacketLimit_) {
                    packetLimit_ <<= 2;
                    LOG(Log::Verbose) << "Packet limit increased to " << packetLimit_; 
                }
            } else {
                LOG(Log::Verbose) << "Mismatch: sent " << sent_ << ", received " << ts.received();
                sent_ = ts.received();
                f_.clear();
                f_.seekg(sent_);
                if (adaptiveSpeed_ && (packetLimit_ > MIN_PACKET_LIMIT)) {
                    packetLimit_ >>= 1;
                    LOG(Log::Verbose) << "Packet limit decreased to " << packetLimit_; 
                }
            }
        }

        void view() {
            LOG(Log::Verbose) << "Opening remote file...";
            t_.viewRemoteFile(streamId_);
        }

        void progressBar() {
            int barWidth = t_.size().first;
            // TODO sometimes terminal size returns 0,0, why? 
            barWidth = (barWidth == 0) ? 37 : (barWidth - 3);
            int progress = (barWidth * sent_) / size_;
            std::cout << "[" << progressBarColor();
            for (int i = 0; i < barWidth; ++i)
                std::cout << ((i <= progress) ? "#" : " ");
            std::cout << "\033[0m]\033[0K\r" << std::flush;
        }

        char const * progressBarColor() {
            if (packetLimit_ == initialPacketLimit_)
                return "\033[32m";
            if (packetLimit_ == MIN_PACKET_LIMIT)
                return "\033[91m";
            return "\033[22m";
        }

        TerminalClient::Sync & t_;
        std::ifstream f_;
        size_t size_;
        size_t sent_;
        size_t streamId_;
        bool adaptiveSpeed_;
        size_t packetSize_;
        size_t packetLimit_;
        size_t initialPacketLimit_;

        static volatile bool Interrupted_;

        static void SIGINT_handler(int signo) {
            MARK_AS_UNUSED(signo);
            Interrupted_ = true;
        }
            
    }; 

    volatile bool RemoteOpen::Interrupted_ = false;

} // namespace tpp


void PrintVersion() {
    std::cout << "RemoteOpen for terminal++, version " << stamp::version << std::endl;
    std::cout << "    commit:   " << stamp::commit << (stamp::dirty ? "*" : "") << std::endl;
    std::cout << "              " << stamp::build_time << std::endl;
    std::cout << "    platform: " << ARCH << " " << ARCH_SIZE << " " << ARCH_COMPILER << " " << ARCH_COMPILER_VERSION << " " << stamp::build << std::endl;
    exit(EXIT_SUCCESS);
}

int main(int argc, char * argv[]) {
    CheckVersion(argc, argv, PrintVersion);
    using namespace tpp;
    try {
        // set log writer to raw mode and enable the default log
        Log::StdOutWriter().setDisplayLocation(false).setDisplayName(false).setDisplayTime(false).setEoL("\033[0K\r\n");
		Log::Enable(Log::StdOutWriter(), { Log::Default()});
        // initialize the configuration
        Config & config = Config::Setup(argc, argv);
        // enable verbose log if selected
        if (config.verbose())
    		Log::Enable(Log::StdOutWriter(), { Log::Verbose()});
        // create the terminal client and transfer the file
        TerminalClient::Sync t{new LocalPTYSlave{}};
        RemoteOpen::Transfer(t, Config::Instance().filename());
        // clear the progressbar
        std::cout << "\033[0K";
        return EXIT_SUCCESS;
    } catch (NackError const & e) {
        std::cerr << "t++ terminal error: " << e.what() << "\033[0K\r\n";
    } catch (TimeoutError const & e) {
        std::cerr << "t++ terminal timeout.\033[0K\r\n";
    } catch (std::exception const & e) {
        std::cout << "\r\n Error: " << e.what() << "\033[0K\r\n";
    } catch (...) {
        std::cout << "\r\n Unspecified errror.\033[0K\r\n";
    }
    return EXIT_FAILURE;
}

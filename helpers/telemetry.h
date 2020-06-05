#pragma once

#include <filesystem>

#include "helpers.h"

#define TELEMETRY HELPERS_NAMESPACE_DECL::Telemetry::TelemetryLog()
#define FATAL_ERROR HELPERS_NAMESPACE_DECL::Telemetry::FatalErrorLog()

HELPERS_NAMESPACE_BEGIN

    /** The telemetry collector and manager. 

        During the execution, telemetry events can be added via the asociated logs. 

     
     */
    class Telemetry {
    public:

        /** Creates the telemetry manager. 
         
            The telemetry uses RAII to make sure that 
         */
        Telemetry(std::function<void(Telemetry &)> handler):
            handler_{handler},
            writer_{nullptr},
            keepAfterClosing_{false} {

        }

        /** Deletes the telemetry manager and cleans up the state. 
         
            If the telemetry record is dirty, 
         */
        ~Telemetry() {
            close();
        }

        /** Opens the telemetry output file without the `.txt` extension.
         
            If the file already exists, appends an index to it. 
         */
        void open(std::string const & filename) {
            ASSERT(writer_ == nullptr);
            std::filesystem::create_directories(std::filesystem::path{filename}.parent_path());
            filename_ = filename + ".txt";
            std::ofstream * s = new std::ofstream{filename_};
            size_t id = 1;
            while (! s->good()) {
                // if the file does not exist, it is an error and should be raised
                if (!PathExists(filename_))
                    THROW(IOError()) << "Unable to create file " << filename_ << " for telemetry";
                do {
                    filename_ = STR(filename << "." << (id++) << ".txt");
                } while (PathExists(filename_));
            }
            writer_ = new Writer(*this, s);
            // add the telemetry log and the first message
            Log::Enable(*writer_, { TELEMETRY, FATAL_ERROR });
            LOG(TELEMETRY) << "Telemetry started, filename " << filename_;
        }

        void close() {
            // delete the writer so that if there is telemetry in a file, the file is released
            if (writer_ != nullptr) {
                delete writer_;
                // call the handler
                handler_(*this);
                // delete the log unless it should be kept
                if (!keepAfterClosing_)
                    std::filesystem::remove(filename_);
                writer_ = nullptr;
            }
        }

        bool keepAfterClosing() const {
            return keepAfterClosing_;
        }

        void setKeepAfterClosing(bool value = true) {
            keepAfterClosing_ = value;
        }

        /** Returns the filename to which the telemetry was written. 
         */
        std::string const & filename() const {
            return filename_;
        }

        void addLog(HELPERS_NAMESPACE_DECL::Log & log) {
            ASSERT(writer_ != nullptr) << "start the telemetry first";
            Log::Enable(*writer_, { log });
        }

        void addLog(std::initializer_list<std::reference_wrapper<Log>> logs) {
            ASSERT(writer_ != nullptr) << "start the telemetry first";
            Log::Enable(*writer_, logs);
        }

        void addLog(std::vector<std::string> const & logs) {
            ASSERT(writer_ != nullptr) << "start the telemetry first";
            for (auto i : logs) {
                Log::Enable(*writer_, { Log::GetLog(i) });
            }
            //Log::Enable(*writer_, logs);
        }

        /** Returns the number of messages in given log that were reported to the telemetry. 
         */
        size_t messages(Log & log) {
            auto i = messageCounts_.find(& log);
            if (i == messageCounts_.end())
                return 0;
            return i->second;
        }

        /** The default log for the telemetry. 
         */
		static HELPERS_NAMESPACE_DECL::Log & TelemetryLog() {
			static HELPERS_NAMESPACE_DECL::Log telemetryLog("TELEMETRY");
			return telemetryLog;
		}

        static HELPERS_NAMESPACE_DECL::Log & FatalErrorLog() {
			static HELPERS_NAMESPACE_DECL::Log fatalErrorLog("FATAL_ERROR");
			return fatalErrorLog;
        }

    private:

        /** Telemetry log writer. 
         */
        class Writer : public Log::OStreamWriter {
        public:
            Writer(Telemetry & telemetry, std::ostream * stream):
                Log::OStreamWriter{*stream},
                telemetry_{telemetry} {
            }

            ~Writer() override {
                // this is safe because the OStreamWriter destructor does not touch the stream at all
                delete & s_;
            }

            std::ostream & beginMessage(Log::Message const & message) override;

        private:

            Telemetry & telemetry_;

        }; // Telemetry::Writer

        void addMessage(Log::Message const & message) {
            auto i = messageCounts_.find(& message.log());
            if (i == messageCounts_.end())
                messageCounts_.insert(std::pair<HELPERS_NAMESPACE_DECL::Log *, size_t>{& message.log(), 1});
            else 
                ++(i->second);
        }

        std::function<void(Telemetry &)> handler_;
        std::unordered_map<HELPERS_NAMESPACE_DECL::Log *, size_t> messageCounts_;

        std::string filename_;
        Writer * writer_;
        bool keepAfterClosing_;

    }; // Telemetry


    inline std::ostream & Telemetry::Writer::beginMessage(Log::Message const & message) {
        telemetry_.addMessage(message);
        return HELPERS_NAMESPACE_DECL::Log::OStreamWriter::beginMessage(message);
    }

HELPERS_NAMESPACE_END

#pragma once

#include <string>
#include <sstream>
#include <fstream>
#include <iostream>
#include <chrono>
#include <mutex>
#include <iomanip>
#include <functional>
#include <vector>
#include <unordered_map>
#include <vector>

#include "helpers.h"

/** \page helpersLog Logging
 
    \brief Basic support for logging stuff to cout, files and more, with small overhead. 

	\section helpersLogArgs Command-line arguments support

 */


#define LOG(...) if (HELPERS_NAMESPACE_DECL::Logger::GetLog(__VA_ARGS__).enabled()) HELPERS_NAMESPACE_DECL::Logger::GetLog(__VA_ARGS__).createMessage(__FILE__, __LINE__)

HELPERS_NAMESPACE_BEGIN

    class Log {
	public:

	    class Message;

	    /** Defines the API for log messages to be written to the log. 
		 */
		class Writer {
		public:
		    virtual std::ostream & beginMessage(Message const & message) = 0;
			virtual void endMessage(Message const & message) = 0;
			virtual ~Writer() {
			}
		};

		/** Log message. 
		 */
	    class Message {
		public:
		    Log & log() const {
				ASSERT(log_ != nullptr);
				return *log_;
			}

			char const * file() const {
				return file_;
			}

			size_t line() const {
				return line_;
			}

			std::time_t const & time() const {
				return time_;
			}

			template<typename T>
			Message & operator << (T const & what) {
				(*s_) << what;
				return *this;
			}

			Message(Message const &) = delete;

			Message(Message && from):
			    log_(from.log_),
				file_(from.file_),
				line_(from.line_),
				time_(from.time_),
				s_(from.s_) {
				from.log_ = nullptr;
			}

			Message & operator = (Message const &) = delete;
			Message & operator = (Message &&) = delete;

			~Message();

		private:
			friend class Log;

			Message(Log * log, char const * file, size_t line);

		    Log * log_;
			char const * const file_;
			size_t const line_;
			std::time_t time_;
			std::ostream * s_;

		}; // Log::Message

		/** Creates new log. 
		 */
		Log(std::string const & name);

		~Log();

	    /** Name of the log. 
		 */
	    std::string const & name() const {
			return name_;
		}

		Writer & writer() const {
			ASSERT(writer_ != nullptr) << "Cannot get writer for disabled log";
			return *writer_;
		}

		void enable(Writer & writer) {
			writer_ = & writer;
		}

		/** Determines whether the log is enabled, or not. 
		 
		    Logging to a disabled log has no effect and negligible performance overhead of checking this flag.
		 */
		bool enabled() const {
			return writer_ != nullptr;
		}

		/** Disables the log by clearing its writer. 
		 */
		void disable() {
			writer_ = nullptr;
		}

		/** Creates new message for the log. 
		 */
		Message createMessage(char const * file, size_t line) {
			ASSERT(writer_ != nullptr) << "Cannot create message for disabled log";
			return Message(this, file, line);
		}

		static Log & Default() {
			static Log defaultLog("");
			return defaultLog;
		}

		static Log & Verbose() {
			static Log verboseLog("VERBOSE");
			return verboseLog;
		}

		static Log & Debug() {
			static Log debugLog("DEBUG");
			return debugLog;
		}

	private:
	    std::string name_;
		Writer * writer_;
	};

	class Logger {
	public:

		/** A simple std::ostream based log message writer. 
		 
		    Writes all log messages into the given stream, allows to specify what parts of the message are to be printed, such as the location in the source, timestamp and logname. 

			The logger is also protected by a mutex so that only one message can be reported at one time. Note that a non-recursive mutex is used so that the application fails if a log message is being generated as part of another log message as this is considered a bad practice. 
		 */
	    class OStreamWriter : public Log::Writer {
		public:

			OStreamWriter(std::ostream & s, bool displayLocation = true, bool displayTime = true, bool displayName = true, std::string eol = "\n"):
			    s_{s},
				displayLocation_{displayLocation},
				displayTime_{displayTime},
				displayName_{displayName},
                eol_{eol} {
			}

		    std::ostream & beginMessage(Log::Message const & message) override {
				// can't use RAII here				
				m_.lock();
				if (displayTime_) {
					tm t;
#ifdef ARCH_WINDOWS
					localtime_s(&t, &message.time());
#else
					localtime_r(&message.time(), &t);
#endif
					s_ << std::put_time(&t, "%c") << " ";
				}
				if (displayName_ && ! message.log().name().empty())
				    s_ << "[" << message.log().name() << "] ";
				return s_;
			}

			void endMessage(Log::Message const & message) override {
				MARK_AS_UNUSED(message);
				if (displayLocation_)
					s_ << " (" << message.file() << ":" << message.line() << ")";
				s_ << eol_ << std::flush;
				// finally, unlock the mutex
				m_.unlock();
			}

            OStreamWriter & setDisplayLocation(bool value = true) {
                displayLocation_ = value;
                return *this;
            }

            OStreamWriter & setDisplayTime(bool value = true) {
                displayTime_ = value;
                return *this;
            }

            OStreamWriter & setDisplayName(bool value = true) {
                displayName_ = value;
                return *this;
            }

            OStreamWriter & setEoL(std::string const & value) {
                eol_ = value;
                return *this;
            }

		protected:
		    std::ostream & s_;
			bool displayLocation_;
			bool displayTime_;
			bool displayName_;
            std::string eol_;
			std::mutex m_;
		}; // Logger::OStreamWriter

		/** Appends the messages to the given filename. 
		 
		    When creates, opens a stream to the provided filename and throws IOError on fialure.
		 */
		class FileWriter : public OStreamWriter {
		public:
		    FileWriter(std::string const & filename, bool displayLocation = true, bool displayTime = true, bool displayName = true, std::string eol = "\n"):
			    OStreamWriter(* new std::ofstream(filename, std::ofstream::app), displayLocation, displayTime, displayName, eol) {
				if (! s_.good())
				    THROW(IOError()) << "Unable to open log file " << filename;
			}

			~FileWriter() {
				// this is safe because the OStreamWriter destructor does not touch the stream at all
				delete & s_;
			}

		}; // Logger::FileWriter

		static Logger::OStreamWriter & StdOutWriter() {
			static OStreamWriter writer(std::cout);
			return writer;
		}

		static void EnableAll(Log::Writer & writer, bool update = false) {
			std::unordered_map<std::string, Log *> & logs = RegisteredLogs();
			for (auto i : logs) {
				if (update || ! i.second->enabled())
				    i.second->enable(writer);
			}
		}

		static void Enable(Log::Writer & writer, std::initializer_list<std::reference_wrapper<Log>> logs) {
			for (auto i : logs)
			    i.get().enable(writer);
		}

		static Log & GetLog() {
		    return Log::Default();
		}

		static Log & GetLog(Log & log) {
			return log;
		}

        static Log & GetLog(Log & (*log)()) {
            return log();
        }

	private:
		friend class Log;
	    
		static std::unordered_map<std::string, Log *> & RegisteredLogs() {
			static std::unordered_map<std::string, Log *> logs;
			return logs;
		}
	}; // Logger

	inline Log::Message::Message(Log * log, char const * file, size_t line):
	    log_(log),
		file_(file),
		line_(line),
		time_(std::chrono::system_clock::to_time_t(std::chrono::system_clock::now())),
		s_(nullptr) {
			s_ = & log_->writer_->beginMessage(*this);
	}

	inline Log::Message::~Message() {
		if (log_ != nullptr)
		    log_->writer_->endMessage(*this);
	}

	/** Creates new log. 
	 */
	inline Log::Log(std::string const & name):
		name_(name),
		writer_(nullptr) {
		std::unordered_map<std::string, Log *> & logs = Logger::RegisteredLogs();
		ASSERT(logs.find(name_) == logs.end()) << "Log " << name_ << " already exists";
		logs.insert(std::make_pair(name_, this));
	}

	inline Log::~Log() {
		std::unordered_map<std::string, Log *> & logs = Logger::RegisteredLogs();
		logs.erase(logs.find(name_));
	}


HELPERS_NAMESPACE_END

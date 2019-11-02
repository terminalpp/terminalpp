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

#define LOG(...) if (::helpers::Logger::GetLog(__VA_ARGS__).enabled()) ::helpers::Logger::GetLog(__VA_ARGS__).createMessage(__FILE__, __LINE__)

namespace helpers {

    class Log {
	public:
	    class Message;

	    /** Defines the API for log messages to be written to the log. 
		 */
		class Writer {
		public:
		    virtual std::ostream & beginMessage(Message const & message) = 0;
			virtual void endMessage(Message const & message) = 0;
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
		Log(std::string const & name):
		    name_(name),
			writer_(nullptr) {
		}

	    /** Name of the log. 
		 */
	    std::string const & name() const {
			return name_;
		}

		Writer & writer() const {
			ASSERT(writer_ != nullptr) << "Cannot get writer for disabled log";
			return *writer_;
		}

		void setWriter(Writer & writer) {
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

	private:
	    std::string name_;
		Writer * writer_;
	};

	class Logger {
	public:
	    class OStreamWriter : public Log::Writer {
		public:

			OStreamWriter(std::ostream & s, bool displayLocation = true, bool displayTime = true, bool displayName = true):
			    s_(s),
				displayLocation_(displayLocation),
				displayTime_(displayTime),
				displayName_(displayName) {
			}

		    std::ostream & beginMessage(Log::Message const & message) override {
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
				if (displayLocation_)
					s_ << " (" << message.file() << ":" << message.line() << ")";

				return s_;
			}

			void endMessage(Log::Message const & message) override {
				MARK_AS_UNUSED(message);
				s_ << std::endl;
			}

		private:
		    std::ostream & s_;
			bool displayLocation_;
			bool displayTime_;
			bool displayName_;
		}; // Logger::OStreamWriter

		static Log::Writer & StdOutWriter() {
			static OStreamWriter writer(std::cout);
			return writer;
		}

		static Log & DefaultLog() {
			static Log defaultLog("");
			return defaultLog;
		}

		static Log & GetLog() {
		    return DefaultLog();
		}

		static Log & GetLog(Log & log) {
			return log;
		}
	};

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

} // namespace helpers

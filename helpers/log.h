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

//#define LOG ::helpers::Log::CreateWriter(__FILE__, __LINE__)
#define LOG if (false) ::helpers::Log::CreateWriter(__FILE__, __LINE__)

namespace helpers {

	class Logger;


	Logger & CerrLogger();

	class Log {
	public:

		/** Log message. 

		    Each message contains information about its origin in the source code, the time at which it was created and its text. It is the responsibility of the selected logger to actually deal with the message according to its own settings. 
		 */
		class Message {
		public:
			char const * file;
			size_t line;
			std::time_t time;
			std::string text;

			Message(char const * file, size_t line, std::string const & text = "") :
				file(file),
				line(line),
				time(std::chrono::system_clock::to_time_t(std::chrono::system_clock::now())),
				text(text) {
			}
		};

		class Writer {
		public:

			Writer(Writer const &) = delete;

			Writer(Writer && from) :
				m_(from.m_),
				logger_(from.logger_),
				s_(std::move(from.s_)) {
				from.logger_ = nullptr;
			}

			/** When the writer object is deleted, the message is finalized and sent to the logger.
			 */
			~Writer();

			/** The call operator allows to select a logger. 
			 */
			Writer & operator () (std::function<Logger &()> logger) {
				logger_ = & logger();
				return *this;
			}

			Writer & operator () (std::string const & loggerName) {
				auto & regLoggers = RegisteredLoggers();
				auto i = regLoggers.find(loggerName);
				if (i == regLoggers.end())
					logger_ = nullptr;
				else
					logger_ = i->second;
				return *this;
			}

			/** Log writer supports all the operations of a stringstream. 
			 */
			template<typename T>
			Writer & operator << (T const & what) {
				if (logger_ != nullptr) {
					s_ << what;
					return *this;
				}
			}

		private:

			friend class Log;

			/** Creates the log writer object for given logger.
			 */
			Writer(char const * file, size_t line, Logger & logger) :
				m_(file, line),
				logger_(& logger) {
			}


			/** The message created with the writer. 
			 */
			Message m_;

			/** Logger (object actually responsible for logging the message). 
			 */
			Logger * logger_;

			/** String stream where the log message is constructed. 
			 */
			std::stringstream s_;

		}; // Log::Writer

		/** Creates new log writer. 

		    The log writer starts with the default cerr logger, but this can be overriden using the `()` operator after the `LOG` macro.
		 */
		static Writer CreateWriter(char const * file, size_t line) {
			return Writer(file, line, CerrLogger());
		}

		/** Registers a logger for given log level. 

		    
		 */
		static void RegisterLogger(Logger * logLevel) {
			Instance().registeredLoggers_.push_back(logLevel);
		}


	private:
		friend class Logger;

		/** Log destructor, which is only called on the singleton in Instance method deletes all registered loggers. 
		 */
		~Log() {
			for (auto l : registeredLoggers_)
				delete l;
		}

		/** All loggers are registered in a map so that they can be selected by their name without the need of having the logger static functions in sight. 
		 */
		static std::unordered_map<std::string, Logger *> & RegisteredLoggers() {
			static std::unordered_map<std::string, Logger *> registeredLoggers;
			return registeredLoggers;
		}

		static Log & Instance() {
			static Log log;
			return log;
		}

		std::vector<Logger *> registeredLoggers_;

	};

	class Logger {
	public:

		/** Each logger has a name.
		 */
		std::string const name;


		virtual void log(Log::Message const & m) = 0;

		/** Destructor removes the logger from the register. 
		 */
		virtual ~Logger() {
			Log::RegisteredLoggers().erase(name);
		}

	protected:
		/** When logger is created, it is registered under its unique name. 
		 */
		Logger(std::string const & name) :
			name(name) {
			auto & regLoggers = Log::RegisteredLoggers();
			auto i = regLoggers.find(name);
			ASSERT(i == regLoggers.end()) << "Logger " << name << " already registered";
			regLoggers[name] = this;
		}
	};

	inline Log::Writer::~Writer() {
		if (logger_ != nullptr) {
			m_.text = s_.str();
			logger_->log(m_);
		}
	}

	class StreamLogger : public Logger {
	public:

		StreamLogger(std::string const & name, std::ostream & s, bool printTime = false, bool printName = true, bool printLocation = false) :
			Logger(name),
			printTime_(printTime),
			printName_(printName),
			printLocation_(printLocation),
			s_(&s) {
		}

		StreamLogger(std::string const & name, bool printTime = false, bool printName = true, bool printLocation = false) :
			Logger(name),
			printTime_(printTime),
			printName_(printName),
			printLocation_(printLocation),
			s_(nullptr) {
		}

		void log(Log::Message const & m) override {
			if (s_ != nullptr) {
				std::lock_guard<std::mutex> g(m_);
				// TODO actually print the message
				if (printTime_)
					(*s_) << std::put_time(std::localtime(&m.time), "%c") << " ";
				if (printName_)
					(*s_) << '[' << name << "] ";
				(*s_) << m.text;
				if (printLocation_)
					(*s_) << " (" << m.file << ":" << m.line << ")";
				(*s_) << std::endl;
			}
		}

		StreamLogger * printTime(bool value = true) {
			printTime_ = value;
			return this;
		}

		StreamLogger * printName(bool value = true) {
			printName_ = value;
			return this;
		}

		StreamLogger * printLocation(bool value = true) {
			printLocation_ = value;
			return this;
		}

		StreamLogger * toFile(std::string const & name) {
			f_.open(name);
			if (!f_.good())
				THROW(IOError(STR("Unable to open file " << name << " for log level " << this->name)));
			s_ = &f_;
			return this;
		}

	protected:

		bool printTime_;
		bool printName_;
		bool printLocation_;

		std::mutex m_;
		std::ostream * s_;
		std::ofstream f_;
	};

	/** Default logger which outputs all messages to the standard error. 
	 */
	inline Logger & CerrLogger() {
		static StreamLogger logger("default", std::cerr, true, false, false);
		return logger;
	}

} // namespace helpers
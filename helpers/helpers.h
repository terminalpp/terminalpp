#pragma once

#ifdef HELPERS_NAMESPACE
#define HELPERS_NAMESPACE_DECL :: HELPERS_NAMESPACE
#define HELPERS_NAMESPACE_BEGIN namespace HELPERS_NAMESPACE {
#define HELPERS_NAMESPACE_END }
#else
#define HELPERS_NAMESPACE_DECL
#define HELPERS_NAMESPACE_BEGIN
#define HELPERS_NAMESPACE_END
#endif

#include <string>
#include <sstream>
#include <exception>
#include <functional>
#include <fstream>
#include <iostream>
#include <chrono>
#include <mutex>
#include <iomanip>
#include <vector>
#include <unordered_map>

#if (defined ARCH_WINDOWS)
    #include <Windows.h>
    #undef OPAQUE
#elif (defined ARCH_UNIX)
	#include <cstring>
	#include <errno.h>
#else
    #error "Unsupported platfrom, only Windows and UNIX like systems supported by helpers"
#endif

#define STR(...) static_cast<std::stringstream &&>(std::stringstream() << __VA_ARGS__).str()

/** Marks given argument as unused so that the compiler will stop giving warnings about it when extra warnings are enabled. 
 */
#define MARK_AS_UNUSED(ARG_NAME) (void)(ARG_NAME)


/** \name Pointer-to-pointer cast
 
    Since any pointer to pointer cast can be done by two static_casts to and from `void *`, this simple template provides a shorthand for that functionality.
 */
//@{
template<typename T, typename W>
inline T pointer_cast(W const * from) {
    return static_cast<T>(static_cast<void const *>(from));
}

template<typename T, typename W>
inline T pointer_cast(W * from) {
    return static_cast<T>(static_cast<void *>(from));
}
//@}

/** \section Exceptions
 
The Exception class is intended to be base class for all exceptions used in the application. It inherits from `std::exception` and provides an accessible storage for the `what()` value. Furthermore each exception also remembers the source file and line where it was raised. 

To facilitate this, exceptions which inherit from `Exception` should not be thrown using `throw` keyword, but the provided `THROW` macro which also patches the exception with the source file and line:
    
    THROW(Exception()) << "An error";

Note that the preferred way of specifying custom error messages is to use the `<<` operator after the `THROW` macro. 

In case an exception needs to be created, but not thrown, a non-throwing constructor macro CREATE_EXCEPTION is provided. 

Convenience macros `NOT_IMPLEMENTED` and `UNREACHABLE` which throw the Exception with a short explanation are provided.

Finally the `PANIC` macro creates an exception, logs its creation, then prints the exception to std::cerr and calls the exit() function immediately. This is useful when the exception based reporting should be used in contexts where exceptions should not be thrown (such as destructors, etc). 
 */

#define THROW(...) throw Log::Exception() && __VA_ARGS__ & HELPERS_NAMESPACE_DECL::Exception::Builder(#__VA_ARGS__, __LINE__,__FILE__)
#define CREATE_EXCEPTION(...) __VA_ARGS__ & HELPERS_NAMESPACE_DECL::Exception::Builder(#__VA_ARGS__, __LINE__, __FILE__)
#define PANIC(...) Exception::Panic{} <<= Log::Exception() && __VA_ARGS__ & HELPERS_NAMESPACE_DECL::Exception::Builder(#__VA_ARGS__, __LINE__, __FILE__)

#define NOT_IMPLEMENTED THROW(HELPERS_NAMESPACE_DECL::Exception()) << "Not implemented code triggered"
#define UNREACHABLE THROW(HELPERS_NAMESPACE_DECL::Exception()) << "Unreachable code triggered"

/** Convenience macro for checking calls which return error using the platform specified way. 

    Executes its arguments and if they evaluate to false, throw OSError. The message of the OS error can be provided after the OSCHECK using the `<<` notion. 
 */
#define OSCHECK(...) if (! (__VA_ARGS__)) THROW(HELPERS_NAMESPACE_DECL::OSError())

HELPERS_NAMESPACE_BEGIN

    class Exception : public std::exception {
    public:
        /** Simple class responsible for storing the exception origin and message.
         */
        class Builder {
        public:

            Builder(char const * exception, size_t line, char const * file):
                exception_{exception},
                line_{line},
                file_{file} {
            }

            template<typename T>
            Builder & operator << (T const & what) {
                what_ << what;
                return *this;
            }

        private:
            friend class Exception;

            char const * exception_;
            size_t line_;
            char const * file_;
            std::stringstream what_;
        }; // Exception::Builder

        /** RAII panic initiator. 
         
            When `<<=` is called on the Panic initiator with an exception, the exception is printed and then the program is terminated with EXIT_FAILURE code. 

            The `<<=` has been selected as the lowest priority operator so that the exception passed to it in the `PANIC` macro can be logged and created as all other exceptions are. 
         */
        class Panic {
        public:
            [[noreturn]] void operator <<= (Exception const & e) {
                std::cerr << "PANIC: " << e;
                exit(EXIT_FAILURE);
            }
        }; 

        char const * what() const noexcept override {
            return what_.c_str();
        }

        char const * exception() const noexcept {
            return exception_;
        }

        /** Sets the message of the exception. 
         
            This method is useful when the exception message is changed after the exception has been thrown, such as when more error context is available. 
         */
        void setMessage(std::string const & what) {
            what_ = what;
        }

        size_t line() const {
            return line_;
        }

        char const * file() const {
            return file_;
        }

    protected:

        char const * exception_ = nullptr;

        std::string what_;

        size_t line_ = 0;
        char const * file_ = nullptr;

    private:
        template<typename T>
        friend T && operator & (T && e, Builder const & binfo) {
            static_assert(std::is_base_of<Exception, T>::value, "Must be derived from ::Exception");
            e.updateWith(binfo);
            return std::forward<T>(e);
        }

		friend std::ostream & operator << (std::ostream & o, Exception const & e) {
            if (e.file() != nullptr)
                o << e.file() << "[" << e.line() << "]: ";
		    o << e.what();
			return o;
		}

        void updateWith(Builder const & binfo) {
            line_ = binfo.line_;
            file_ = binfo.file_;
            what_ = what_ + binfo.what_.str();
            exception_ = binfo.exception_;
        }

    };

HELPERS_NAMESPACE_END

/** \section Common Exception Classes
 * 
 * 
 */

HELPERS_NAMESPACE_BEGIN

    class OSError : public Exception {
    public:
        OSError() {
#if (defined ARCH_WINDOWS)
            what_ = STR("ErrorCode: " << GetLastError());
#elif (defined ARCH_UNIX)
            what_ = STR(strerror(errno) << ": ");
#else
    	    what_ = "OS Error";
#endif
        }
    }; // OSError

	class IOError : public Exception {
	};

    class TimeoutError : public Exception {
    };

HELPERS_NAMESPACE_END

/** \section Assertions
    
    Instead of `c` style `assert`, the macro `ASSERT` which uses the exceptions mechanism is provided. The `ASSERT` macro takes the condition as an argument and can be followed by help message using the standard `<<` `std::ostream` operator. If it fails, the `AssertionError` exception is thrown:

        ASSERT(x == y) << "x and y are not the same, ouch - x: " << x << ", y: " << y;

    If `NDEBUG` is specified, `ASSERT` translates to dead code and will be removed by the optimizer. 

    The `ASSERT_PANIC` macro works as `ASSERT`, but instead of raising the assect exception, terminates immediately. This is useful when the assertion mechanics should be used in contexts where exceptions are not supported, such as destructors. 
 */
#ifdef NDEBUG 
    #define ASSERT(...) if (false) std::stringstream()
    #define ASSERT_PANIC(...) if (false) std::stringstream()
#else
    #define ASSERT(...) if (! (__VA_ARGS__)) THROW(HELPERS_NAMESPACE_DECL::AssertionError(#__VA_ARGS__))
    #define ASSERT_PANIC(...) if (! (__VA_ARGS__)) PANIC(HELPERS_NAMESPACE_DECL::AssertionError(#__VA_ARGS__))
#endif

HELPERS_NAMESPACE_BEGIN

	class AssertionError : public Exception {
	public:
		explicit AssertionError(char const * code):
			Exception() {
            what_ = STR("Assertion failure: (" << code << ") "); 
		}
	};

HELPERS_NAMESPACE_END

/** \page helpersLog Logging
 
    \brief Basic support for logging stuff to cout, files and more, with small overhead. 


 */

#define LOG(...) if (HELPERS_NAMESPACE_DECL::Log::GetLog(__VA_ARGS__).enabled()) HELPERS_NAMESPACE_DECL::Log::GetLog(__VA_ARGS__).createMessage(__FILE__, __LINE__)

HELPERS_NAMESPACE_BEGIN


    class Log {
	public:

	    class Message;

	    /** Defines the API for log messages to be written to the log. 
		 */
		class Writer {
		public:

            /** If a writer is deleted, it must detach from all active logs first. 
             */
            virtual ~Writer();

		    virtual std::ostream & beginMessage(Message const & message) = 0;
			virtual void endMessage(Message const & message) = 0;
		}; // Log::Writer

        class OStreamWriter;
        class FileWriter;

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
			std::time_t time_ = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
			std::ostream * s_ = nullptr;

		}; // Log::Message

		/** Creates new log. 
		 */
		explicit Log(std::string const & name);

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

        template<typename T>
        T && operator && (T && e) {
            static_assert(std::is_base_of<HELPERS_NAMESPACE_DECL::Exception, T>::value, "Must be derived from ::Exception");
            if (enabled())
                createMessage(e.file(), e.line()) << e.exception() << ": " << e.what();
            return std::forward<T>(e);
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

        static Log & Exception() {
            static Log exceptionLog("EXCEPTION");
            return exceptionLog;
        }

        static OStreamWriter & StdOutWriter();

		static void EnableAll(Log::Writer & writer, bool update = false) {
			std::unordered_map<std::string, Log *> const & logs = RegisteredLogs();
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

        static Log & GetLog(std::function<Log &()> log) {
            return log();
        }

        static Log & GetLog(std::string const & name) {
            auto & logs = RegisteredLogs();
            auto i = logs.find(name);
            if (i == logs.end())
                THROW(HELPERS_NAMESPACE_DECL::Exception()) << "Log " << name << " not registered";
            return *(i->second);
        }

	private:

	    std::string name_;

		Writer * writer_ = nullptr;

		static std::unordered_map<std::string, Log *> & RegisteredLogs() {
			static std::unordered_map<std::string, Log *> logs;
			return logs;
		}
	};

    /** A simple std::ostream based log message writer. 
     
        Writes all log messages into the given stream, allows to specify what parts of the message are to be printed, such as the location in the source, timestamp and logname. 

        The logger is also protected by a mutex so that only one message can be reported at one time. Note that a non-recursive mutex is used so that the application fails if a log message is being generated as part of another log message as this is considered a bad practice. 
        */
    class Log::OStreamWriter : public Log::Writer {
    public:

        OStreamWriter(std::ostream & s, bool displayLocation = true, bool displayTime = true, bool displayName = true, std::string const & eol = "\n"):
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
    }; // Log::OStreamWriter

    /** Appends the messages to the given filename. 
     
        When creates, opens a stream to the provided filename and throws IOError on fialure.
        */
    class Log::FileWriter : public Log::OStreamWriter {
    public:
        FileWriter(std::string const & filename, bool displayLocation = true, bool displayTime = true, bool displayName = true, std::string const & eol = "\n"):
            OStreamWriter{* new std::ofstream(filename, std::ofstream::app), displayLocation, displayTime, displayName, eol} {
            if (! s_.good())
                THROW(IOError()) << "Unable to open log file " << filename;
        }

        ~FileWriter() override {
            // this is safe because the OStreamWriter destructor does not touch the stream at all
            delete & s_;
        }


    }; // Log::FileWriter

    inline Log::Writer::~Writer() {
        auto logs = Log::RegisteredLogs();
        for (auto i : logs)
            if (i.second->writer_ == this)
                i.second->writer_ = nullptr;
    }

	inline Log::Message::Message(Log * log, char const * file, size_t line):
	    log_{log},
		file_{file},
		line_{line} {
			s_ = & log_->writer_->beginMessage(*this);
	}

	inline Log::Message::~Message() {
		if (log_ != nullptr)
		    log_->writer_->endMessage(*this);
	}

	/** Creates new log. 
	 */
	inline Log::Log(std::string const & name):
		name_{name} {
		std::unordered_map<std::string, Log *> & logs = RegisteredLogs();
		ASSERT(logs.find(name_) == logs.end()) << "Log " << name_ << " already exists";
		logs.insert(std::make_pair(name_, this));
	}

	inline Log::~Log() {
		std::unordered_map<std::string, Log *> & logs = RegisteredLogs();
		logs.erase(logs.find(name_));
	}

    inline Log::OStreamWriter & Log::StdOutWriter() {
        static OStreamWriter writer(std::cout);
        return writer;
    }

HELPERS_NAMESPACE_END


HELPERS_NAMESPACE_BEGIN

    inline bool CheckVersion(int argc, char ** argv, std::function<void()> versionPrinter) {
        if (argc == 2 && strcmp("--version", argv[1]) == 0) {
            versionPrinter();
            return true;
        }
        return false;
    }

HELPERS_NAMESPACE_END


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

#if (defined ARCH_WINDOWS)
    #include <windows.h>
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


template<typename T, typename W>
inline T pointer_cast(W const * from) {
    return static_cast<T>(static_cast<void const *>(from));
}

template<typename T, typename W>
inline T pointer_cast(W * from) {
    return static_cast<T>(static_cast<void *>(from));
}

/** \section Exceptions
 
The Exception class is intended to be base class for all exceptions used in the application. It inherits from `std::exception` and provides an accessible storage for the `what()` value. Furthermore, unless `NDEBUG` macro is defined, each exception also remembers the source file and line where it was raised. 

To facilitate this, exceptions which inherit from `Exception` should not be thrown using `throw` keyword, but the provided `THROW` macro which also patches the exception with the source file and line, if appropriate:
    
    THROW(Exception()) << "An error";

Note that the preferred way of specifying custom error messages is to use the `<<` operator after the `THROW` macro. 

In case an exception needs to be created, but not thrown, a non-throwing constructor macro CREATE_EXCEPTION is provided. 

Convenience macros `NOT_IMPLEMENTED` and `UNREACHABLE` which throw the Exception with a short explanation are provided.

 */

#ifdef NDEBUG 
#define THROW(...) throw __VA_ARGS__ & HELPERS_NAMESPACE_DECL::Exception::ExceptionInfo()
#define CREATE_EXCEPTION(...) __VA_ARGS__ & HELPERS_NAMESPACE_DECL::Exception::ExceptionInfo()
#define ASSERT(...) if (false) std::stringstream()

#else
#define THROW(...) throw __VA_ARGS__ & HELPERS_NAMESPACE_DECL::Exception::ExceptionInfo(__LINE__,__FILE__)
#define CREATE_EXCEPTION(...) __VA_ARGS__ & HELPERS_NAMESPACE_DECL::Exception::ExceptionInfo(__LINE__, __FILE__)
#define ASSERT(...) if (! (__VA_ARGS__)) THROW(HELPERS_NAMESPACE_DECL::AssertionError(#__VA_ARGS__))
#endif

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
        class ExceptionInfo {
        public:

        #ifdef NDEBUG
            ExceptionInfo() {
            }
        #else
            ExceptionInfo(size_t line, char const * file):
                line_(line),
                file_(file) {
            }
        #endif

            template<typename T>
            ExceptionInfo & operator << (T const & what) {
                what_ << what;
                return *this;
            }

        private:
            friend class Exception;

        #ifndef NDEBUG
            size_t line_;
            char const * file_;
        #endif

            std::stringstream what_;
        };

        Exception() {
        }

        char const * what() const noexcept override {
            return what_.c_str();
        }

        /** Sets the message of the exception. 
         
            This method is useful when the exception message is changed after the exception has been thrown, such as when more error context is available. 
         */
        void setMessage(std::string const & what) {
            what_ = what;
        }

        #ifndef NDEBUG
        size_t line() const {
            return line_;
        }

        char const * file() const {
            return file_;
        }
        #endif

    protected:

        std::string what_;

        #ifndef NDEBUG
            size_t line_ = 0;
            char const * file_ = nullptr;
        #endif


    private:
        template<typename T>
        friend T && operator & (T && e, ExceptionInfo const & cinfo) {
            static_assert(std::is_base_of<Exception, T>::value, "Must be derived from ::Exception");
            e.updateWith(cinfo);
            return std::move(e);
        }

		friend std::ostream & operator << (std::ostream & o, Exception const & e) {
            #ifndef NDEBUG
                if (e.file() != nullptr)
                    o << e.file() << "[" << e.line() << "]: ";
            #endif
		    o << e.what();
			return o;
		}

        void updateWith(ExceptionInfo const & cinfo) {
            #ifndef NDEBUG
                line_ = cinfo.line_;
                file_ = cinfo.file_;
            #endif
            what_ = what_ + cinfo.what_.str();
        }

    };
HELPERS_NAMESPACE_END

/** \section Common Exception Classes
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
 */
HELPERS_NAMESPACE_BEGIN

	class AssertionError : public Exception {
	public:
		AssertionError(char const * code):
			Exception() {
            what_ = STR("Assertion failure: (" << code << ") "); 
		}
	};

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

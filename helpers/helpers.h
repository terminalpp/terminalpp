#pragma once

#include <string>
#include <sstream>

#ifdef _WIN64
    #include "windows.h"
#elif (defined __linux__) || (defined __APPLE__)
    #include <cstring>
    #include <errno.h>
#else
    #error "Unsupported platfrom, only Win32 and Linux are supported by helpers."
#endif

#define STR(...) static_cast<std::stringstream &&>(std::stringstream() << __VA_ARGS__).str()

/** Marks given argument as unused so that the compiler will stop giving warnings about it when extra warnings are enabled. 
 */
#define MARK_AS_UNUSED(ARG_NAME) (void)(ARG_NAME)

#ifdef NDEBUG 
#define THROW(...) throw __VA_ARGS__ & ::helpers::Exception::ExceptionInfo()
#define ASSERT(...) if (false) std::stringstream()
#else
#define THROW(...) throw __VA_ARGS__ & ::helpers::Exception::ExceptionInfo(__LINE__,__FILE__)
#define ASSERT(...) if (! (__VA_ARGS__)) THROW(::helpers::AssertionError(#__VA_ARGS__))
#endif

#define NOT_IMPLEMENTED THROW(::helpers::Exception()) << "Not implemented code triggered"
#define UNREACHABLE THROW(::helpers::Exception()) << "Unreachable code triggered"

/** Convenience macro for checking calls which return error using the platform specified way. 

    Executes its arguments and if they evaluate to false, throw OSError. The message of the OS error can be provided after the OSCHECK using the `<<` notion. 
 */
#define OSCHECK(...) if (! (__VA_ARGS__)) THROW(::helpers::OSError())

namespace helpers {

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
            static_assert(std::is_base_of<Exception, T>::value, "Must be derived from ::helpers::Exception");
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

	class AssertionError : public Exception {
	public:
		AssertionError(char const * code):
			Exception() {
            what_ = STR("Assertion failure: (" << code << ") "); 
		}
	};


    class OSError : public Exception {
    public:
        OSError() {
            #ifdef _WIN64
                what_ = STR("ErrorCode: " << GetLastError());
            #elif __linux__
                what_ = STR(strerror(errno) << ": ");
            #endif
        }
    }; // helpers::OSError


	class IOError : public Exception {
	};


















	/** Shorthand for determining whether value is in given inclusive interval. 
	 */
	template<typename T>
	bool InRangeInclusive(T const & what, T const & fromValue, T const & toValue) {
		return (what >= fromValue && what <= toValue);
	}



} // namespace helpers

#pragma once
#include <string>
#include <sstream>

#if (defined ARCH_WINDOWS)
    #include "windows.h"
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

#ifdef NDEBUG 
#define THROW(...) throw __VA_ARGS__ & ::helpers::Exception::ExceptionInfo()
#define CREATE_EXCEPTION(...) __VA_ARGS__ & ::helpers::Exception::ExceptionInfo()
#define ASSERT(...) if (false) std::stringstream()

#else
#define THROW(...) throw __VA_ARGS__ & ::helpers::Exception::ExceptionInfo(__LINE__,__FILE__)
#define CREATE_EXCEPTION(...) __VA_ARGS__ & ::helpers::Exception::ExceptionInfo(__LINE__, __FILE__)
#define ASSERT(...) if (! (__VA_ARGS__)) THROW(::helpers::AssertionError(#__VA_ARGS__))
#endif

#define NOT_IMPLEMENTED THROW(::helpers::Exception()) << "Not implemented code triggered"
#define UNREACHABLE THROW(::helpers::Exception()) << "Unreachable code triggered"

/** Convenience macro for checking calls which return error using the platform specified way. 

    Executes its arguments and if they evaluate to false, throw OSError. The message of the OS error can be provided after the OSCHECK using the `<<` notion. 
 */
#define OSCHECK(...) if (! (__VA_ARGS__)) THROW(::helpers::OSError())

namespace helpers {

    template<typename T, typename W>
    inline T pointer_cast(W const * from) {
        return static_cast<T>(static_cast<void const *>(from));
    }

    template<typename T, typename W>
    inline T pointer_cast(W * from) {
        return static_cast<T>(static_cast<void *>(from));
    }

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
#if (defined ARCH_WINDOWS)
            what_ = STR("ErrorCode: " << GetLastError());
#elif (defined ARCH_UNIX)
            what_ = STR(strerror(errno) << ": ");
#else
    	    what_ = "OS Error";
#endif
        }
    }; // helpers::OSError

	class IOError : public Exception {
	};

    class TimeoutError : public Exception {
    };

#ifdef ARCH_WINDOWS
	/** 
	 */
	class Win32Handle {
	public:

		HANDLE& operator * () {
			return h_;
		}

		HANDLE* operator -> () {
			return &h_;
		}

		Win32Handle() :
			h_(INVALID_HANDLE_VALUE) {
		}

		Win32Handle(HANDLE h) :
			h_(h) {
		}

		~Win32Handle() {
			close();
		}

		void close() {
			if (h_ != INVALID_HANDLE_VALUE) {
				CloseHandle(h_);
				h_ = INVALID_HANDLE_VALUE;
			}
		}

		operator HANDLE () {
			return h_;
		}

		operator HANDLE* () {
			return &h_;
		}
	private:
		HANDLE h_;
	};



#endif

	/** Shorthand for determining whether value is in given inclusive interval. 
	 */
	template<typename T>
	bool InRangeInclusive(T const & what, T const & fromValue, T const & toValue) {
		return (what >= fromValue && what <= toValue);
	}

    template<typename T>
    T ClipToRange(T const & value, T const & min, T const & max) {
        if (value < min)
            return min;
        if (value > max)
            return max;
        return value;
    }

} // namespace helpers

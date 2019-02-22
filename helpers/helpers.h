#pragma once

#include <string>
#include <sstream>

#define STR(...) static_cast<std::stringstream &&>(std::stringstream() << __VA_ARGS__).str()

#ifdef NDEBUG 
#define THROW(...) do {  throw __VA_ARGS__; } while (false)
#define ASSERT(...) if (false) std::stringstream()
#else
#define THROW(...) do { auto i = __VA_ARGS__; i.setOrigin(__LINE__, __FILE__); throw i; } while (false)
#define ASSERT(...) if (! (__VA_ARGS__)) throw ::helpers::AssertionError(__LINE__, __FILE__, #__VA_ARGS__)
#endif

#define NOT_IMPLEMENTED THROW(::helpers::Exception("Not implemented code triggered"))
#define UNREACHABLE THROW(::helpers::Exception("Unreachable code triggered"))

namespace helpers {

	class Exception : public std::exception {
	public:
#ifdef NDEBUG
		Exception() {
		}

		Exception(std::string const & what) :
			what_(what) {
		}

		friend std::ostream & operator << (std::ostream & o, Exception const & e) {
			o << "error: " << e.what();
			return o;
		}

#else
		Exception() :
			line_(0),
			file_(nullptr) {
		}


		Exception(std::string const & what) :
			what_(what),
			line_(0),
			file_(nullptr) {
		}

		size_t line() const {
			return line_;
		}

		char const * file() const {
			return file_;
		}

		void setOrigin(size_t line, char const * file) {
			line_ = line;
			file_ = file;
		}

		friend std::ostream & operator << (std::ostream & o, Exception const & e) {
			if (e.file() != nullptr)
				o << e.file() << "[" << e.line() << "]: ";
		    o << "error: " << e.what();
			return o;
		}
#endif

		char const * what() const noexcept override {
			return what_.c_str();
		}

	protected:
		std::string what_;

#ifndef NDEBUG
		size_t line_;
		char const * file_;
#endif
	}; // helpers::Exception

	class AssertionError : public Exception {
	public:
		AssertionError(std::string const & what):
			Exception(STR("Assertion failure (" << what << ") ")) {
		}

#ifndef NDEBUG
		AssertionError(size_t line, char const * file, std::string const & what):
			Exception(STR("Assertion failure (" << what << ") ")) {
			setOrigin(line, file);
		}
#endif

		template<typename T>
		AssertionError & operator << (T const & what) {
			what_ = STR(what_ << what);
			return *this;
		}
	};



} // namespace helpers
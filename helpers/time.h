#pragma once
#define __STDC_WANT_LIB_EXT1__ 1

#include <chrono>
#include <ctime>

#include "helpers.h"

namespace helpers {

	inline std::string TimeInISO8601() {
		time_t now;
		time(&now);
		std::string result("2011-10-08T07:07:09Z");
#ifdef __STDC_SECURE_LIB__
		struct tm buf;
		OSCHECK(gmtime_s(&buf, &now) == 0);
		strftime(& result[0], result.size() + 1, "%FT%TZ", &buf);
#else
		strftime(&result[0], result.size() + 1, "%FT%TZ", gmtime(&now));
#endif
		return result;
	}

	/** Returns a steady clock time in milliseconds. 

	    Note that the time has no meaningful reference, so all it is good for is simple time duration calculations. 
	 */
	inline size_t SteadyClockMillis() {
		using namespace std::chrono;
		return static_cast<size_t>(
			duration<long, std::milli>(
				duration_cast<milliseconds>(
					steady_clock::now().time_since_epoch()
					)
				).count()
			);
	}

	/** Simple timer class which encapsulates the std::chrono.
	 */
	class Timer {
	public:
		Timer():
		    started_(false),
		    value_(0) {
		}

		~Timer() {
		}

		void start() {
			started_ = true;
			start_ = std::chrono::steady_clock::now();
		}

		/** Returns the duration in seconds. 
		 */
		size_t stop() {
			ASSERT(started_) << "Timer was not started";
			auto end = std::chrono::steady_clock::now();
			value_ = static_cast<size_t>(std::chrono::duration<long, std::milli>(std::chrono::duration_cast<std::chrono::milliseconds>(end - start_)).count());
			started_ = false;
			return value_;
		}

		size_t value() const {
			return value_;
		}

	private:
		bool started_;
		std::chrono::steady_clock::time_point start_;
		size_t value_;
	};



} // namespace helpers
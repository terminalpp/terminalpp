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

	/** Simple timer class which encapsulates the std::chrono.
	 */
	class Timer {
	public:
		Timer():
		    started_(false) {
		}

		~Timer() {
		}

		void start() {
			started_ = true;
			start_ = std::chrono::high_resolution_clock::now();
		}

		/** Returns the duration in seconds. 
		 */
		double stop() {
			ASSERT(started_) << "Timer was not started";
			value_ = std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::high_resolution_clock::now() - start_);
			started_ = false;
			return value_.count();
		}

	private:
		bool started_;
		std::chrono::high_resolution_clock::time_point start_;
		std::chrono::duration<double> value_;
	};



} // namespace helpers
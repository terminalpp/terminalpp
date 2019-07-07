#pragma once

#include <chrono>

#include "helpers.h"

namespace helpers {

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
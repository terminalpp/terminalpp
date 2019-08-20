#pragma once
#define __STDC_WANT_LIB_EXT1__ 1

#include <chrono>
#include <ctime>
#include <atomic>
#include <mutex>
#include <thread>

#include "helpers.h"
#include "object.h"

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
        struct tm buf;
        gmtime_r(&now, &buf);
		strftime(&result[0], result.size() + 1, "%FT%TZ", &buf);
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

	/** Simple stopwatch class which encapsulates the std::chrono and calculates time intervals in milliseconds.
	 
	    TODO change to use SteadyClockMillis
	 */
	class Stopwatch {
	public:
		Stopwatch():
		    started_(false),
		    value_(0) {
		}

		~Stopwatch() {
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

	class Timer;

	typedef EventPayload<bool, Timer> TimerEvent;

	/** A simple timer class.

	    Triggers the onTimer event in specified interval, or recurrently. The event timer itself may change whether the timer should continue, or not by changing its value.

		Synchronization is a bit involved and uses a mutex and a compare and swap on the thread's activity. The lock is held by the timer, while the activity flag is held by the thread, which allows for safe stopping and destruction of the timer without the need to wait the specified interval. See start & stop methods for details. 
	 */
	class Timer : public Object {
	public:
	    Event<TimerEvent> onTimer;

		Timer():
		    killSwitch_(nullptr),
			active_(false) {
		}

		~Timer() override {
			stop();
		}

		void start(size_t durationMs, bool recurrent = true) {
			std::lock_guard<std::mutex> g(m_);
			ASSERT(active_ == false) << "Timer already running";
			// join the finished thread if necessary
			if (t_.joinable())
			    t_.join();
			active_ = true;
			t_ = std::thread([this, durationMs, recurrent](){
				int expectedActive = 1;
				std::atomic<int> active(true);
				killSwitch_ = & active;
				while (active == 1) {
					std::this_thread::sleep_for(std::chrono::milliseconds(durationMs));
					TimerEvent e(this, recurrent);
					// cas active to 2 so that timer can't delete itself and the event while we use it
					if (active.compare_exchange_strong(expectedActive, 2)) {
						std::lock_guard<std::mutex> g(m_);
    					onTimer.trigger(e);
						// if between cas and the lock timer wanted to stop, its killSwitch will be nullptr, and it will be waiting for us to terminate (i.e. set to inactive), otherwise set to active
						active = killSwitch_ == nullptr ? 0 : expectedActive;
					}
					if (! *e)
					    break;
				}
				// if we can CAS active to inactive, we must inform it that we have terminated by setting killSwitch to nullptr (active to false just to be on safe side).
				if (active.compare_exchange_strong(expectedActive, 0)) {
					std::lock_guard<std::mutex> g(m_);
					killSwitch_ = nullptr;
					active_ = false;
				}
			});
			// a simple busy-wait before the thread fills in the killSwitch ptr
			while (killSwitch_ == nullptr)
				std::this_thread::yield();
		}

		void stop() {
			{
				// first obtain lock and then check if active, if not just make sure to join the thread if necessary
				std::lock_guard<std::mutex> g(m_);
				if (active_ == false) {
					// make sure the thread is joined, if possible (or we get abortion)
					if (t_.joinable())
					    t_.join();
				    return;
			    }
				int active = 1;
				// if active, check if we can CAS the thread to deactivate, and if we can, the thread will ignore timer update when it exits and therefore we have to set timer killSwitch to nullptr and active to false to show no-one runs and detach the thread so that it exits when it feels like (guaranteed it won't touch this ptr)
				if (killSwitch_->compare_exchange_strong(active, 0)) {
					t_.detach();
					active_ = false;
					killSwitch_ = nullptr;
					return;
				}
				// if not successful in the CAS, set killswitch to nullptr and join with the terminating thread, after which we set the active to false. KillSwitch is set to null inside the lock to tell the disable to the thread (in case its active settings is 2, i.e. currently triggering)
    			killSwitch_ = nullptr;
			}
			t_.join();
			active_ = false;
		}

		bool running() {
			return active_;
		}

	private:

		std::mutex m_;
	    std::thread t_;

		volatile std::atomic<int> * killSwitch_;

		bool active_ = false;

	};



} // namespace helpers
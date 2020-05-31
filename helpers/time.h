#pragma once
#define __STDC_WANT_LIB_EXT1__ 1

#include <chrono>
#include <ctime>
#include <atomic>
#include <mutex>
#include <thread>
#include <functional>

#include "helpers.h"

HELPERS_NAMESPACE_BEGIN

	// TODO - time pretty printer
	inline std::string PrettyPrintMillis(size_t millis) {
		return STR(millis);
	}

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


	/** Very simple timer class.
	 
	    Executes the given handler with given duration. The handler is a function taking no arguments and returning bool. If the handlers returns true, it will be rescheduled after the specified interval, otherwise the timer will be stopped. 
	 */
	class Timer {
	public:
		Timer():
		    data_{new Data()} {
			data_->attach();
		}

		~Timer() {
			stop();
			data_->detach();
		}

		bool running() const {
			std::lock_guard<std::mutex> g(data_->m);
			return data_->running;
		}

		size_t interval() const {
			std::lock_guard<std::mutex> g(data_->m);
			return data_->interval;
		}

		void setInterval(size_t ms) {
			std::lock_guard<std::mutex> g(data_->m);
			data_->interval = ms;
		}

		void setHandler(std::function<bool()> handler) {
			std::lock_guard<std::mutex> g(data_->m);
			data_->handler = handler;
		}

		/** Starts the timer. 
		 
		    If the timer is already running, terminates the old thread first. This is perhaps not completely efficient, but saves us a lot of concurrency worries. 
		 */
		void start() {
			std::lock_guard<std::mutex> g(data_->m);
			if (data_->running)
			    ++data_->threadId;
			size_t tid = data_->threadId;
			data_->attach();
			Data * data = data_;
			std::thread t{[data, tid](){
				size_t interval = 0;
				std::function<bool()> handler;
				while (true) {
					{
						std::lock_guard<std::mutex> g(data->m);
						// we have been stopped, exit the thread
						if (data->threadId != tid)
						    return;
						interval = data->interval;
						handler = data->handler;
					}
					// if the handler indicates termination, terminate the thread
					if (! handler()) {
						std::lock_guard<std::mutex> g(data->m);
						// indicate we have stopped and exit the thread
						if (data->threadId == tid)
						    data->running = false;
						return;
					}
					// otherwise sleep for the requested period
					std::this_thread::sleep_for(std::chrono::milliseconds(interval));
				}
				data->detach();
			}};
			t.detach();
			data_->running = true;
		}

		/** Stops the timer. 
		 */
		void stop() {
			std::lock_guard<std::mutex> g(data_->m);
			if (data_->running)
			    ++data_->threadId;
		}

	private:

		/** Internal data of the timer. 
		 
		    The data is allocated on heap and is reference counted, which is a simple way to make sure that if the timer is deleted we do not have to wait for the timer thread to terminate as well (this could be painful if the interval is large). 
		 */
	    class Data {
		public:
		    std::mutex m;
		    volatile size_t threadId;
			volatile size_t interval;
			bool running;
    	    std::function<bool()> handler;

			Data():
			    threadId{0},
				interval{1000},
				running{false},
				ptrCount_{0} {
			}

			void attach() {
				++ptrCount_;
			}

			void detach() {
				m.lock();
				--ptrCount_;
				if (--ptrCount_ == 0) {
					m.unlock();
				    delete this;
				} else {
				    m.unlock();
				}
			}
		private:
			size_t ptrCount_;

		};

		Data * data_;

	}; 

HELPERS_NAMESPACE_END
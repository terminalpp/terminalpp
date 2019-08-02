#pragma once

#include <mutex>
#include <atomic>
#include <condition_variable>


namespace helpers {

    /** A simple RAII pointer to a class providing unlock() method which is called when the pointer goes out of scope.
     */
    template<typename T>
    class SmartRAIIPtr {
    public:
        SmartRAIIPtr(T & value, bool lock = true):
            value_( &value) {
            if (lock)
                value.lock();
        }

        SmartRAIIPtr(SmartRAIIPtr && from) noexcept:
            value_(from.value_) {
            from.value_ = nullptr;
        }

        ~SmartRAIIPtr() {
            if (value_ != nullptr)
                value_->unlock();
        }

        SmartRAIIPtr & operator = (SmartRAIIPtr && from) {
            if (value_ != nullptr)
                value_->unlock();
            value_ = from.value_;
            from.value_ = nullptr;
            return * this;
        }

        SmartRAIIPtr(SmartRAIIPtr const &) = delete;
        SmartRAIIPtr & operator = (SmartRAIIPtr const &) = delete;

        T * release() {
            T * result = value_;
            value_ = nullptr;
            return result;
        }

        T const & operator * () const {
            ASSERT(value_ != nullptr);
            return *value_;
        }

        T & operator * () {
            ASSERT(value_ != nullptr);
            return *value_;
        }

        T const * operator -> () const {
            ASSERT(value_ != nullptr);
            return value_;
        }

        T * operator -> () {
            ASSERT(value_ != nullptr);
            return value_;
        }

    private:

        T * value_;

    }; // helpers::SmartRAIIPtr


    /** A simple lock that allows locking in normal and priority modes, guaranteeing that a priority lock request will be serviced before any waiting normal locks. 
     
     */
    class PriorityLock {
    public:

        PriorityLock():
            priorityRequests_(0) {
        }

        /** Grabs the lock in non-priority mode.
         */
        void lock() {
            std::unique_lock<std::mutex> g(m_);
            while (priorityRequests_ > 0)
                cv_.wait(g);
            g.release();
        }

        /** Grabs the lock in priority mode.
         */
        void priorityLock() {
            ++priorityRequests_;
            m_.lock();
            --priorityRequests_;
        }

        /** Releases the lock. 
         */
        void unlock() {
            cv_.notify_all();
            m_.unlock();
        }

    private:

        std::atomic<unsigned> priorityRequests_;
        std::mutex m_;
        std::condition_variable cv_;
    }; 

} // namespace helpers
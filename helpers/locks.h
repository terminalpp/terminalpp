#pragma once

#include <mutex>
#include <thread>
#include <atomic>
#include <condition_variable>

HELPERS_NAMESPACE_BEGIN

    /** A simple lock that allows locking in normal and priority modes, guaranteeing that a priority lock request will be serviced before any waiting normal locks. 
     */
    class PriorityLock {
    public:

        PriorityLock():
            priorityRequests_(0) {
        }

        /** Grabs the lock in non-priority mode.
         */
        PriorityLock & lock() {
            std::unique_lock<std::mutex> g(m_);
            while (priorityRequests_ > 0)
                cv_.wait(g);
            g.release();
#ifndef NDEBUG
            locked_ = std::this_thread::get_id();
#endif
            return *this;
        }

        /** Grabs the lock in priority mode.
         */
        PriorityLock & priorityLock() {
            ++priorityRequests_;
            m_.lock();
            --priorityRequests_;
#ifndef NDEBUG
            locked_ = std::this_thread::get_id();
#endif
            return *this;
        }

        /** Releases the lock. 
         */
        void unlock() {
            cv_.notify_all();
#ifndef NDEBUG
            locked_ = std::thread::id{};
#endif
            m_.unlock();
        }

#ifndef NDEBUG
        bool locked() const {
            return locked_ == std::this_thread::get_id();
        }
#endif

    private:
        std::atomic<unsigned> priorityRequests_;
        std::mutex m_;
        std::condition_variable cv_;
#ifndef NDEBUG
        std::atomic<std::thread::id> locked_;
#endif          
    }; 

    /** Reentrant variant of the priority lock. 
     */
    class ReentrantPriorityLock {
    public:

        ReentrantPriorityLock():
            depth_{0},
            priorityRequests_(0) {
        }

        /** Grabs the lock in non-priority mode.
         */
        ReentrantPriorityLock & lock() {
            if (owner_ != std::this_thread::get_id()) {
                std::unique_lock<std::mutex> g(m_);
                while (priorityRequests_ > 0)
                    cv_.wait(g);
                g.release();
                owner_ = std::this_thread::get_id();
            }
            ++depth_;
            return *this;
        }

        /** Grabs the lock in priority mode.
         */
        ReentrantPriorityLock & priorityLock() {
            ++priorityRequests_;
            if (owner_ != std::this_thread::get_id()) {
                m_.lock();
                owner_ = std::this_thread::get_id();
            }
            --priorityRequests_;
            ++depth_;
            return *this;
        }

        /** Releases the lock. 
         */
        void unlock() {
            ASSERT(owner_ == std::this_thread::get_id());
            ASSERT(depth_ > 0);
            if (--depth_ == 0) {
                owner_ = std::thread::id{};
                cv_.notify_all();
                m_.unlock();
            }        
        }

    private:
        std::thread::id owner_;
        unsigned depth_;
        std::atomic<unsigned> priorityRequests_;
        std::mutex m_;
        std::condition_variable cv_;
    }; 


HELPERS_NAMESPACE_END
#pragma once

#include "widget.h"
#include "renderer.h"

namespace ui {

    /** CRTP renderer which manages a single static events queue per renderer class. 
     */
    template<typename T>
    class TypedEventQueue : public EventQueue {
    public:

        void schedule(std::function<void()> event, Widget * widget) override {
            Schedule(event, widget);
        }

        bool processEvent() override {
            return ProcessEvent();
        }

        void cancelEvents(Widget * widget) override {
            std::lock_guard<std::mutex> g{eventsGuard_};
            if (PendingEvents(widget) == 0)
                return;
            for (auto & e : events_) {
                if (e.second == widget)
                    e.first = nullptr;
                if (--PendingEvents(widget) == 0)
                    break;
            }
        }

        static void Schedule(std::function<void()> event, Widget * widget) {
            std::lock_guard<std::mutex> g{eventsGuard_};
            events_.push_back(std::make_pair(event, widget));
            if (widget != nullptr)
                ++PendingEvents(widget);;
        }

        static bool ProcessEvent() {
            std::function<void()> handler;
            {
                std::lock_guard<std::mutex> g{eventsGuard_};
                while (true) {
                    if (events_.empty())
                        return false;
                    auto e = events_.front();
                    events_.pop_front();
                    if (! e.first)
                        continue;
                    if (e.second != nullptr)
                        --PendingEvents(e.second);
                    handler = e.first;
                    break;
                }
            }
            handler();
            return true;
        }

    private:

        static std::deque<std::pair<std::function<void()>, Widget*>> events_;
        static std::mutex eventsGuard_;

    }; 

    template<typename T>
    std::deque<std::pair<std::function<void()>, Widget*>> TypedEventQueue<T>::events_;

    template<typename T>
    std::mutex TypedEventQueue<T>::eventsGuard_;

}
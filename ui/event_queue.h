#pragma once

#include "widget.h"

namespace ui {

    /** CRTP renderer which manages a single static events queue per renderer class. 
     */
    template<typename T>
    class EventQueue {
    protected:

        void schedule(std::function<void()> event, Widget * widget) {
            std::lock_guard<std::mutex> g{eventsGuard_};
            events_.push_back(std::make_pair(event, widget));
            if (widget != nullptr)
                ++widget->pendingEvents_;
        }

        static void ProcessEvent() {
            std::function<void()> handler;
            {
                std::lock_guard<std::mutex> g{eventsGuard_};
                while (true) {
                    if (events_.empty())
                        return;
                    auto e = events_.front();
                    events_.pop_front();
                    if (! e.first)
                        continue;
                    if (e.second != nullptr)
                        -- (e.second->pendingEvents_);
                    handler = e.first;
                    break;
                }
                handler();
            }
        }

    private:

        void cancelWidgetEvents(Widget * widget) {
            std::lock_guard<std::mutex> g{eventsGuard_};
            if (widget->pendingEvents_ == 0)
                return;
            for (auto & e : events_)
                if (e.second == widget)
                    e.first = nullptr;
        }

        static std::deque<std::pair<std::function<void()>, Widget*>> events_;
        static std::mutex eventsGuard_;

    }; 

    template<typename T>
    std::deque<std::pair<std::function<void()>, Widget*>> EventQueue<T>::events_;

    template<typename T>
    std::mutex EventQueue<T>::eventsGuard_;

}
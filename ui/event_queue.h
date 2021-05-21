#pragma once

#include "widget.h"

namespace ui {

    /** \section Event Scheduling
     
        Inside the UI, event scheduling is a shared responsibility of both the ui::Widget and the ui::Renderer classes. Each event *must* be attached to a widget, and each widget remembers how many pending events it has currently scheduled so that if the widget is detached or deleted, its events can be cancelled (otherwise the events may hold pointers and references to the already dead structures). 

        A widget can schedule its own event as long as it is attached to a renderer via the ui::Widget::schedule() method. Such event will be linked to the widget automatically. Renderer can be used to schedule events for any widget via its ui::Renderer::schedule() method. Furthermore, a renderer can schedule event not linked to any widget (this is implemented by linking the event to a dummy widget each renderer creates for its own lifetime). 

        Each renderer is given an event queue reference when created that is used to schedule its events. The event queue is decoupled from renderer so that multiple renderers can use the same event queue (such as multiple GUI windows of the same application). 
        
        The actual renderer implementation should override the ui::Renderer::schedule() method to inform the real main thread that an event has been scheduled so that it can later call the ui::EventQueue::processEvent() method to execute the event. 
     */

    /** Event queue for widgets. 

        Implements an event queue that is capable of scheduling arbitrary code to be executed in the main thread. Each event is tied to a widget and when scheduled, the widget's number of pending events is increased. Similarly, when the event is executed, the corresponding number of pending events is decreased. 

        All events of a given widget can be cancelled, typically when the widget is deleted, or detached from its renderer. 

        The event queue only deals with UI events. Depending on the renderer used, it may have its own main thread implementation and event queue. The renderer must make sure that any UI events scheduled in the UI event queue (this class) are correctly integrated in its own event queue and are executed when appropriate by calling the processEvent() method.  
     */
    class EventQueue {
    public:
        /** Schedules new event linked to the specified widget. 
         
            The widget must not be nullptr. Can be called from any thread. 
         */
        void schedule(std::function<void()> event, Widget * widget) {
            ASSERT(widget != nullptr);
            std::lock_guard<std::mutex> g{eventsGuard_};
            events_.push_back(std::make_pair(event, widget));
            if (widget != nullptr)
                ++widget->pendingEvents_;
        }

        /** Processes previously scheduled event. 
         
            Skips over cancelled events until it finds valid event, which is removed from the queue and executed. If there are no valid events in the queue, returns immediately. 

            Must be called from the main thread. 
         */
        bool processEvent() {
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
                        --(e.second->pendingEvents_);
                    handler = e.first;
                    break;
                }
            }
            handler();
            return true;
        }

        /** Invalidates all events linked to the given widget. 
         
            The widget must not be nullptr. Can be called from any thread. 
         */
        void cancelEvents(Widget * widget) {
            ASSERT(widget != nullptr);
            std::lock_guard<std::mutex> g{eventsGuard_};
            if (widget->pendingEvents_ == 0)
                return;
            for (auto & e : events_) {
                if (e.second == widget) {
                    e.first = nullptr;
                    if (--(widget->pendingEvents_) == 0)
                        break;
                }
            }
        }

    private:
        /** The event queue. 
         */
        std::deque<std::pair<std::function<void()>, Widget*>> events_;
        /** Event queue guard for multithreaded access. '
         */
        std::mutex eventsGuard_;
        
    }; // ui::EventQueue

}
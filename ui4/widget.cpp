#include "widget.h"

namespace ui {

    Widget * Widget::appendChild(Widget * w) {
        ASSERT(IN_UI_THREAD);
        if (w->parent_ != nullptr)
            if (w->parent_ == this) {
                swapChildren(w, frontChild_);
                return w;
            } else {
                w->parent_->removeChild(w);
            }
        // note the prev/next look reversed because the list is paint order, but the user interface is in visibility order (i.e. reversed paint order). So the previous sibling of the added widget is the previous front        
        if (backChild_ == nullptr) {
            backChild_ = w;
        } else {
            frontChild_->nextSibling_ = w;
            w->previousSibling_ = frontChild_;
        }
        frontChild_ = w;
        w->parent_ = this;
        // TODO call the attach event

        return w;
    }

    Widget * Widget::removeChild(Widget * w) {
        ASSERT(IN_UI_THREAD);
        ASSERT(w->parent_ == this);
        w->parent_ = nullptr;
        if (backChild_ == w)
            backChild_ = w->nextSibling_;
        else
            w->previousSibling_->nextSibling_ = w->nextSibling_;
        if (frontChild_ == w)
            frontChild_ = w->previousSibling_;
        else
            w->nextSibling_->previousSibling_ = w->previousSibling_;
        // TODO call the detach event
        return w;
    }

    void Widget::swapChildren(Widget * a, Widget * b) {
        ASSERT(IN_UI_THREAD);
        ASSERT(a->parent_ == this);
        ASSERT(b->parent_ == this);
        if (a == nullptr || b == nullptr || a == b)
            return;
        std::swap(a->previousSibling_, b->previousSibling_);
        std::swap(a->nextSibling_, b->nextSibling_);
        if (backChild_ == a)
            backChild_ = b;
        else if (backChild_ == b)
            backChild_ = a;
        if (frontChild_ == a)
            frontChild_ = b;
        else if (frontChild_ == b)
            frontChild_ = a;
    }

    void Widget::updateVisibleRectangle() {
        ASSERT(IN_UI_THREAD);
        // in theory we can also return empty immediately if the parent visible rect is empty, but this actually calculates the offset of the widget to the buffer, which might be beneficial
        if (parent_ == nullptr)
            visibleRect_.rect = Rect::Empty();
        else 
            visibleRect_ = parent_->visibleRect_.offsetBy(parent_->scrollOffset_).clip(rect_); 
        for (auto i : *this)
            i->updateVisibleRectangle();
    }

    // event scheduling

    Widget * Widget::GlobalEventDummy_ = new Widget{};
    std::deque<std::pair<std::function<void()>, Widget *>> Widget::Events_;
    std::mutex Widget::EventsGuard_;

    void Widget::cancelEvents() {
        std::lock_guard<std::mutex> g{EventsGuard_};
        if (pendingEvents_ == 0)
            return;
        for (auto & e : Events_) {
            if (e.second == this) {
                e.second = nullptr;
                if (--pendingEvents_ == 0)
                    break;
            }
        }
    }

    void Widget::Schedule(std::function<void()> event, Widget * sender) {
        ASSERT(sender != nullptr);
        std::lock_guard<std::mutex> g{EventsGuard_};
        Events_.push_back(std::make_pair(event, sender));
        ++(sender->pendingEvents_);
    }

    bool Widget::ProcessEvent() {
        ASSERT(IN_UI_THREAD);
        std::function<void()> handler;
        {
            std::lock_guard<std::mutex> g{EventsGuard_};
            while (true) {
                if (Events_.empty())
                    return false;
                    auto e = Events_.front();
                    Events_.pop_front();
                    // if the event has been cancelled, move to the next event, if any
                    if (e.second == nullptr)
                        continue;
                    handler = e.first;
                    ASSERT(e.second->pendingEvents_ > 0);
                    --(e.second->pendingEvents_);
                    break;
            }
        }
        handler();
        return true;
    }



} // namespace ui
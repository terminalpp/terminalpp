#include <unordered_set>

#include "widget.h"

namespace ui {

    Widget * Widget::CommonParent(Widget * a, Widget * b) {
        ASSERT(IN_UI_THREAD);
        if (a == nullptr)
            return b;
        if (b == nullptr)
            return a;
        std::unordered_set<Widget *> parents;
        for (; a != nullptr; a = a->parent_)
            parents.insert(a);
        for (; b != nullptr; b = b->parent_)
            if (parents.find(b) != parents.end())
                return b;
        return nullptr; 
    }

    Widget * Widget::appendChild(Widget * w) {
        ASSERT(IN_UI_THREAD);
        if (w->parent_ != nullptr)
            if (w->parent_ == this) {
                w->moveToFront();
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
        detach(w);
        w->parent_ = nullptr;
        // TODO call the detach event
        return w;
    }

    void Widget::detach(Widget * child) {
        ASSERT(IN_UI_THREAD);
        ASSERT(child->parent_ == this);
        if (child->previousSibling_ != nullptr)
            child->previousSibling_->nextSibling_ = child->nextSibling_;
        if (child->nextSibling_ != nullptr)
            child->nextSibling_->previousSibling_ = child->previousSibling_;
        if (backChild_ == child)
            backChild_ = child->nextSibling_;
        if (frontChild_ == child)
            frontChild_ = child->previousSibling_;
        child->previousSibling_ = nullptr;
        child->nextSibling_ = nullptr;
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

    // painting 

    void Widget::update() {
        ASSERT(IN_UI_THREAD);
        Widget * target = this;
        // if the widget is not attached, or the visible rectangle is empty, there is no need to repaint
        if (renderer_ == nullptr || visibleRect_.empty())
            return;
        // otherwise see if we should delegate the repaint to one of the parents
        while (true) {
            if (! target->visible_)
                return;
            if (target->parent_ != nullptr && target->delegateRepaintTarget()) {
                target = target->parent();
                // mark the new target as repaint pending, if already marked, terminate as that widget's repaint will repaint us as well
                if (target->repaintPending_.exchange(true))
                    return;
            } else {
                break;
            }
        }
        // tell renderer to repaint
        renderer_->updateWidget(target);
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
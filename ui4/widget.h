#pragma once

#include <atomic>
#include <vector>
#include <deque>

#include "helpers/helpers.h"

#include "core/event.h"
#include "core/canvas.h"
#include "renderer.h"

namespace ui {



    class Widget {
        friend class Canvas;
        friend class Renderer;
    public:

        /** Creates a widget with empty name. 
         */
        Widget() = default;

        /** Creates a widget with given name. 
         */
        Widget(std::string_view name):
            name_{name} {
        }
#if defined NDEBUG
        virtual ~Widget() {
#else
        virtual ~Widget() noexcept(false) {
#endif
            ASSERT(IN_UI_THREAD);
            // make sure there are no pending events for the widget
            cancelEvents();
            while (frontChild_ != nullptr)
                delete removeChild(frontChild_);
        }

        std::string const & name() const {
            return name_;
        }

        virtual void setName(std::string const & name) {
            name_ = name;
        }

    private:
        std::string name_;

    /**\name Widget tree. 
     
       Widgets are oragnized into a tree. Each widget may have child widgets which are called siblings and form a doubly linked list. 
     
     */
    //@{
    public:

        /** Simple iterator into widget's children. 
         */
        class ChildIterator {
            friend class Widget;
        public:

            using value_type = Widget *;
            using pointer = Widget* const *;
            using reference = Widget* const &;

            ChildIterator(ChildIterator const & from) = default;

            bool operator == (ChildIterator const & other) const {
                return child_ == other.child_;
            }

            bool operator != (ChildIterator const & other) const {
                return child_ != other.child_;
            }

            ChildIterator & operator ++ () {
                if (child_ != nullptr)
                    child_ = child_->nextSibling_;
                return *this;
            }

            ChildIterator operator ++(int) {
                Widget * old = child_;
                if (child_ != nullptr)
                    child_ = child_->nextSibling_;
                return ChildIterator{old};
            }

            reference operator * () const {
                return child_;
            } 

            pointer operator -> () const {
                return &child_;
            }

        private:

            explicit ChildIterator(Widget * child):
                child_{child} {
            }

            Widget * child_;
        };

        /** The parent widget. 
         
            For detached and root widgets this is nullptr, otherwise returns the widget whose child the current widget is. 
         */
        Widget * parent() const {
            ASSERT(IN_UI_THREAD);
            return parent_;
        }

        /** Returns the sibling that is painted just before the current widget. 
         
            If the current widget is painted first amongst its siblings (the widget is in the back), returns nullptr. 
         */
        Widget * previousSibling() const {
            ASSERT(IN_UI_THREAD);
            return previousSibling_;
        }

        /** Returns the sibling that is painted immediately after the current widget. 
         
            If the current widget is painted last amongst its siblings (the widget is in the front), returns nullptr.
         */

        Widget * nextSibling() const {
            ASSERT(IN_UI_THREAD);
            return nextSibling_;
        }

        /** Appends the given widget as a child and returns it
         
            If the widget already has a parent it is first removed and then appended. The widget is appended in the front position, i.e. the last widget to be painted. 

            If the given widget is already a child, it is simply moved to the front.
         */
        Widget * appendChild(Widget * w);

        /** Removes given child and returns it. 
         
            The widget must already be a child. 
         */
        Widget * removeChild(Widget * w);

        /** Returns the child in the back. 
         
            This is the child to be painted first. Returns nullptr if there are no children. 
         */
        Widget * backChild() const {
            ASSERT(IN_UI_THREAD);
            return backChild_;
        }

        /** Returns the child in the front. 
         
            This is the child painted last. Returns nullptr if there are no children. 
         */
        Widget * frontChild() const {
            ASSERT(IN_UI_THREAD);
            return frontChild_;
        }

        /** Returns the start iterator to the children.
         
            Children are accessed in the order of their painting, i.e. the back element first. 
         */ 
        ChildIterator begin() const {
            ASSERT(IN_UI_THREAD);
            return ChildIterator{backChild_};
        }

        /** Returns the iterator to the end of the children. 
         * 
            Children are accessed in the order of their painting, i.e. the back element first, the front child is the last element. This returns the iterator *after* the range. 
         */
        ChildIterator end() const {
            ASSERT(IN_UI_THREAD);
            return ChildIterator{nullptr};
        }

        /** Moves the current widget to the front position within its parent. 
         
            This means the widget will be painted last. Does nothing if already at front, or if the widget is not attached. 
         */
        void moveToFront() {
            ASSERT(IN_UI_THREAD);
            if (parent_ != nullptr)
                parent_->swapChildren(this, parent_->frontChild_);
        }

        /** Moves the current widget to back within its parent. 
         
            This means the widget will be painted first. Does nothiong if already at back, or if the widget is not attached. 
         */
        void moveToBack() {
            ASSERT(IN_UI_THREAD);
            if (parent_ != nullptr)
                parent_->swapChildren(this, parent_->backChild_);
        }

        /** Moves the widget one step later in the paint order. 
         
            If the widget is already painted last (in the front position), or if the widget is not attached, does nothing. 
         */
        void moveForward() {
            ASSERT(IN_UI_THREAD);
            if (parent_ != nullptr)
                parent_->swapChildren(this, this->nextSibling_);
        }

        /** Moves the widget one step sooner in the paint order. 
         
            If the widget is already painted first (in the back position), or if the widget is not attached, does nothing.
         */
        void moveBackward() {
            ASSERT(IN_UI_THREAD);
            if (parent_ != nullptr)
                parent_->swapChildren(this, this->previousSibling_);
        }

    protected:

        /** Swaps the paint order of given child widgets. 
         
            Both widgets must be children. The front and back widgets are updated when necessary. If one of the widgets is nullptr, of if they are the same, does nothing.
         */
        void swapChildren(Widget * a, Widget * b);

    private:
        Widget * parent_ = nullptr;
        Widget * backChild_ = nullptr;
        Widget * frontChild_ = nullptr;
        Widget * previousSibling_ = nullptr;
        Widget * nextSibling_ = nullptr;

    //@}

    /**\name Position, size and layout. 
     
       - simple: no contents size, this can be infinite in theory, only offset
       
     */
    //@{
    public:

        Rect const & rect() const {
            ASSERT(IN_UI_THREAD);
            return rect_;
        }

        Size size() const {
            ASSERT(IN_UI_THREAD);
            return rect_.size();
        }

        int width() const {
            ASSERT(IN_UI_THREAD);
            return rect_.width();
        }

        int height() const {
            ASSERT(IN_UI_THREAD);
            return rect_.height();
        }

        Point topLeft() const {
            ASSERT(IN_UI_THREAD);
            return rect_.topLeft();
        }

        Point topRight() const {
            ASSERT(IN_UI_THREAD);
            return rect_.topRight();
        }

        Point bottomLeft() const {
            ASSERT(IN_UI_THREAD);
            return rect_.bottomLeft();
        }

        Point bottomRight() const {
            ASSERT(IN_UI_THREAD);
            return rect_.bottomRight();
        }

    protected:

    private:

        /** Updates the visible rectangle of the widget and its children.

         */
        void updateVisibleRectangle();

        /** Visible rectangle of the widget. 
         
            The rectangle is with respect to the widget's canvas as opposed to its contents canvas, which can be larger and scrolled. 
         */
        Canvas::VisibleRect visibleRect_; 

        /** The rectangle of the widget wrt its parent's contents. 
         */
        Rect rect_;

        /** Scroll offset of the widget. 
         */
        Point scrollOffset_;

        
    //@}

    /**\name Painting
     
       
     */
    //@{
    public:

        /** Requests the repaint of the widget. 
         
            Schedules a repaint of the widget in the UI thread. If a repaint has already been requested, does nothing. The UI thread then performs further checks, such as whether the parent widget should be repainted instead, and so on. 

            For more details on this, see TODO TODO
         */
        void requestRepaint() {
            if (repaintPending_.exchange(true) == false) {
                schedule([this](){
                    // now we are in the UI thread, determine the real target of the paint

                });
            }
        }

        /** Returns true if the widget is visible. 
         
            Visible widgets will paint themselves when requested.
         */
        bool visible() const {
            ASSERT(IN_UI_THREAD);
            return visible_;
        }

        virtual void setVisible(bool value) {
            ASSERT(IN_UI_THREAD);
            if (visible_ != value) {
                visible_ = value;
                // TODO do stuff
            }
        }

    protected:

        /** The method responsible for actually painting the widget on given canvas. 
         
            This is always called in the UI thread with a valid canvas and the sole purpose of this method is to immediately draw the contents of the widget and exit. No other processing should happen here. 

            The default implementation simply pains the child widgets. 
         */
        virtual void paint(Canvas & canvas) {
            MARK_AS_UNUSED(canvas);
            ASSERT(IN_UI_THREAD);
            for (auto child : *this)
                paintChild(child);
        }

        /** Performs immediate repaint of given child.

            This method is intended to be called from the paint() method so that own children can be painted. Only visible children with non-empty visible rectangles are painted. 
         */
        void paintChild(Widget * child) {
            ASSERT(IN_UI_THREAD);
            ASSERT(child->parent_ == this);
            if (!child->visible() || child->visibleRect_.empty())
                return;
            child->repaintPending_ = false;
            Canvas canvas{child};
            child->paint(canvas);
        }

    private:

        Renderer * renderer_ = nullptr;

        std::atomic<bool> repaintPending_ = false;

        bool visible_ = true;

    //@}

    /**\name Mouse Events
     */
    //@{

    public:

    protected:

        //virtual void mouseDownCapture() = 0;
        //virtual void mouseDownBubble() = 0;


    //@}

    /**\name Event Scheduling
     
       The scheduling of events is supported by all widgets so that a non-UI thread can schedule code to be executed in the main thread. Each event is associated with its sender widget and a widget keeps track of how many events pending their execution already currently exist. When a widget dies, any remaining events associated with it are scrapped and will not be executed. This is useful as a very primitive form of resource management - an event can assume the widget exists and use it freely. 

       For global events that do not need to rely on any widget, the Schedule() static method can be used instead. Under the hood, the global events are assigned with a Widget sentinel global object that never dies. 

       While the scheduling of the events is provided by the widgets, it is the renderer who determines when to run the events via the loop function. 

       TODO that the events system assumes single main loop for the entire application as all events share same queue. If this ever proves an issue, events can be associated with rendereres via their widgets and events belonging to a different renderer can be skipped when executed, but for now this seems as an unnecessary complication. 
     */
    //@{
    public:

        /** Schedules the given function to be executed in the UI thread. 
         
            This function can be called from any thread and the event is associated with current widget as its sender. If the current widget is deleted before the function will get executed, the function will be forgotten. 
         */
        void schedule(std::function<void()> event) {
            Schedule(event, this);
        }

        /** Schedules the given function to be executed in the UI thread. 
         
            The function is not bound to any widget and so its execution will never be cancelled. The event must thus make sure in its own ways that any data it operates on still exist.
         */
        static void Schedule(std::function<void()> event) {
            Schedule(event, GlobalEventDummy_);
        }

    private:

        /** Disables all events with current widget as sender. 
         
            This is automatically called when the widget is being deleted.
         */
        void cancelEvents();

        // Pending events for a widget. The value is protected by the EventsGuard_ mutex as well as the global events queue. 
        unsigned pendingEvents_ = 0;

        /** Schedules event with given widget as sender. 
         
            Sender widget must never be nullptr. 
         */
        static void Schedule(std::function<void()> event, Widget * sender);

        /** Processes single event from the event queue. 
         
            Returns true if an event was found, false if the event queue is empty. 
         */
        static bool ProcessEvent();

        // dummy widget used as sender for global events not attached to any widget
        static Widget * GlobalEventDummy_;

        // the events queue
        static std::deque<std::pair<std::function<void()>, Widget *>> Events_;

        // mutex guarding the event queue
        static std::mutex EventsGuard_;

    //}

    private:
        FRIEND_TEST(ui_widget, events);

    }; // ui::Widget



} // namespace ui
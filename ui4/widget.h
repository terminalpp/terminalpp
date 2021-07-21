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

        /** Returns the closest common parent of the two widgets. 
         
            Nullptr if the widgets are not related at all. 
         */
        static Widget * CommonParent(Widget * a, Widget * b);

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
         
            This means the widget will be painted last and be visible over any of its siblings. Does nothing if already at front, or if the widget is not attached. 
         */
        void moveToFront() {
            ASSERT(IN_UI_THREAD);
            if (parent_ != nullptr && parent_->frontChild_ != this) {
                parent_->detach(this);
                previousSibling_ = parent_->frontChild_;
                previousSibling_->nextSibling_ = this;
                parent_->frontChild_ = this;
            }
        }

        /** Moves the current widget to back within its parent. 
         
            This means the widget will be painted first. Does nothiong if already at back, or if the widget is not attached. 
         */
        void moveToBack() {
            ASSERT(IN_UI_THREAD);
            if (parent_ != nullptr && parent_->backChild_ != this) {
                parent_->detach(this);
                nextSibling_ = parent_->backChild_;
                nextSibling_->previousSibling_ = this;
                parent_->backChild_ = this;
            }
        }

        /** Moves the widget one step later in the paint order (therefore more visible)
         
            If the widget is already painted last (in the front position), or if the widget is not attached, does nothing. 
         */
        void moveForward() {
            ASSERT(IN_UI_THREAD);
            if (parent_ != nullptr && nextSibling_ != nullptr) {
                Widget * after = nextSibling_;
                parent_->detach(this);
                nextSibling_ = after->nextSibling_;
                if (after->nextSibling_ != nullptr)
                    after->nextSibling_->previousSibling_ = this;
                after->nextSibling_ = this;
                this->previousSibling_ = after;
                if (parent_->frontChild_ == after)
                    parent_->frontChild_ = this;
            }
        }

        /** Moves the widget one step sooner in the paint order.
         
            If the widget is already painted first (in the back position), or if the widget is not attached, does nothing.
         */
        void moveBackward() {
            ASSERT(IN_UI_THREAD);
            if (parent_ != nullptr && previousSibling_ != nullptr) {
                Widget * before = previousSibling_;
                parent_->detach(this);
                previousSibling_ = before->previousSibling_;
                if (before->previousSibling_ != nullptr)
                    before->previousSibling_->nextSibling_ = this;
                before->previousSibling_ = this;
                this->nextSibling_ = before;
                if (parent_->backChild_ == before)
                    parent_->backChild_ = this;
            }
        }

    private:

        /** Detaches the given widget from the list, without actually detaching it. 
         
            Expects the widget will be reattached in a different position immediately afterwards.  
         */
        void detach(Widget * child);

        Widget * parent_ = nullptr;
        Widget * backChild_ = nullptr;
        Widget * frontChild_ = nullptr;
        Widget * previousSibling_ = nullptr;
        Widget * nextSibling_ = nullptr;

    //@}

    /**\name Layout
     
       - resize
       - autosized elements

       

     */
    //@{
    public:

    protected:


    private:
    //@}

    /**\name Position and size. 
     
       A widget has its own canvas, which is indefinite and whose origin starts at the top-left corner of the widget. Generally, the usable area of the canvas has the size of the widget itself.  
     
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

    /** \name Updating
      
        When a widget needs to be updated (repainted), the following should happen:

        - Widget update is initiated by calling the requestUpdate() method that schedules the call to update() method in the main thread. Alternatively, the update method can be called immediately from the main thread. Unless immediate repaints are of concern, requestRepaint() is preffered as ignores any new update requests as long as a repaint is pending. 
        - the update() method performs further checking and determines the actual update target by nested calls to delegateRepaintTarget() method, which can delegate the update of a widget to its parent. If a pending update is encountered during the check the current update is stopped (the scheduled update will show the same information). If a valid target is found, the method instructs the renderer to update the widget by calling the Renderer::updateWidget() method. 
        - the renderer then decides, based on its fps settings, whether to immediately update the given widget, or whether buffer the request and wait for the fps trigger to do the repaint. If buffered, the renderer keeps the closest common parent of all widgets that requested update within the timeframe and then repaints it. Immediate or buffered repaints are doen by calling the Renderer::repaint() method.
        - the Renderer::repaint() method takes the widget to repaint as an argument and simply calls the Widget::Repaint() method for the widget. The method in renderer exists only so that it can be overriden in specific renderers so that they can react on the update as the repaint only changes the renderer's buffer which then needs to be rendered. 
        - Widget::Repaint() method clears the pending repaint flag for the widget (thus allowing further repaint requests to be scheduled), creates the canvas for the widget and calls its paint() method. 
        - the paint() method is responsible for actually drawing the contents of the widget. The default implementation simply paints the widgets. This is done by calling the Widget::Repaint() method on the children.
        
     */
    //@{
    public:

        /** Requests the update (repaint) of the widget. 
         
            Schedules an update of the widget in the UI thread. Can be called from any thread. If the widget has already a pending repaint request, returns immediately. Otherwise a repaint request is scheduled in the main thread. 
         */
        void requestUpdate() {
            if (repaintPending_.exchange(true) == false) {
                schedule([this](){
                    update();
                });
            }
        }

        /** Updates the widget immediately-ish. 

            First determines the actual update target (the current widget, or one of its parents). Then calls the renderer to repaint the widget. The renderer then decides whether to repaint immediately, or wait, depending on the fps settings. For more details see the Renderer::repaint() method.
         */
        void update();

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

        /** Immediately draws the given widget on the renderer's buffer. 
         
            NOTE This is a static function because calling protected methods is only allowed via this pointer, not on others. The static repaint function does not suffer from this limitation.
         */
        static void Repaint(Widget * widget) {
            ASSERT(IN_UI_THREAD);
            ASSERT(widget->renderer_ != nullptr);
            widget->repaintPending_ = false;
            Canvas canvas{widget};
            widget->paint(canvas);
        }

        /** The method responsible for actually painting the widget on given canvas. 
         
            This is always called in the UI thread with a valid canvas and the sole purpose of this method is to immediately draw the contents of the widget and exit. No other processing should happen here. 

            The default implementation simply pains the child widgets. 
         */
        virtual void paint(Canvas & canvas) {
            MARK_AS_UNUSED(canvas);
            ASSERT(IN_UI_THREAD);
            for (auto child : *this)
                Repaint(child);
        }

        /** Returns true if the widget's repaint should be delegated to its parent instead. 
         */
        virtual bool delegateRepaintTarget() const {
            return false;
        }

    private:

        /** Determines whether there is a pending repaint in the main thread queue. 
         */
        std::atomic<bool> repaintPending_ = false;

        Renderer * renderer_ = nullptr;

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
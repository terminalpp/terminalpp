#pragma once

#include <deque>

#include "helpers/helpers.h"
#include "helpers/locks.h"
#include "helpers/time.h"

#include "canvas.h"

namespace ui3 {

    class Widget;

    // ============================================================================================


    // ============================================================================================

    /** Base class for all UI renderer implementations. 
     
        
     */ 
    class Renderer {
        friend class Canvas;
        friend class Widget;
    public:

        using Buffer = Canvas::Buffer;
        using Cell = Canvas::Cell;

        virtual ~Renderer() {
            if (renderer_.joinable()) {
                fps_ = 0;
                renderer_.join();
            }
        }

    protected:

        Renderer(Size const & size):
            buffer_{size} {
            }

        Renderer(std::pair<int, int> const & size):
            Renderer(Size{size.first, size.second}) {
        }


    // ============================================================================================
    /** \name Events & Scheduling
     */
    //@{
    public:
        /** Schedule the given event in the main UI thread.
         
            This function can be called from any thread. 
         */
        void schedule(std::function<void()> event, Widget * widget = nullptr);

        void yieldToUIThread();

    protected:

        void processEvent();

    private:

        /** Notifies the main thread that an event is available. 
         */
        virtual void eventNotify() = 0;

        void cancelWidgetEvents(Widget * widget);

        std::deque<std::pair<std::function<void()>, Widget*>> events_;
        std::mutex eventsGuard_;

        std::mutex yieldGuard_;
        std::condition_variable yieldCv_;

    //@}
    // ============================================================================================

    /** \name Widget Tree
     */
    //@{

    public: 
    
        Widget * root() const {
            return root_;
        }

        virtual void setRoot(Widget * value);

    private:
        virtual void widgetDetached(Widget * widget) {
            if (renderWidget_ == widget)
                renderWidget_ = nullptr;
            // TODO this should actually do stuff such as checking that mouse and keyboard focus remains valid, etc. 
            NOT_IMPLEMENTED;
        }

        /** Detaches the subtree of given widget from the renderer. 
         */
        void detachTree(Widget * root) {
            detachWidget(root);
            // now that the whole tree has been detached, fix keyboard & mouse issues, etc. 
            NOT_IMPLEMENTED;
        }

        /** Detaches the given widget by invalidating its visible area, including its entire subtree, when detached, calls the widgetDetached() method.
         */
        void detachWidget(Widget * widget);

        Widget * root_ = nullptr;

    //@}

    // ============================================================================================

    /** \name Layouting and Painting
     */
    //@{
    public:

        Size const & size() const {
            return buffer_.size();
        }

    protected: 

        /** Resizes the renderer. 
         
         */
        virtual void resize(Size const & value);

        unsigned fps() const {
            return fps_; // only UI thread can change fps, no need to lock
        }

        virtual void setFps(unsigned value) {
            if (fps_ == value)
                return;
            if (fps_ == 0) {
                fps_ = value;
                startRenderer(); 
            } else {
                fps_ = value;
            }
        }

        /** Returns the visible area of the entire renderer. 
         */
        Canvas::VisibleArea visibleArea() {
            return Canvas::VisibleArea{this, Point{0,0}, Rect{buffer_.size()}};
        }

    private:

        virtual void render(Buffer const & buffer, Rect const & rect) = 0;

        /** Paints the given widget. 

            */
        void paint(Widget * widget);

        void paintAndRender();

        void startRenderer();

        Buffer buffer_;
        Widget * renderWidget_{nullptr};
        std::atomic<unsigned> fps_{0};
        std::thread renderer_;

    //@}


    }; // ui::Renderer

} // namespace ui
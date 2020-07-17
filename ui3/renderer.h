#pragma once

#include <deque>

#include "helpers/helpers.h"
#include "helpers/locks.h"
#include "helpers/time.h"

#include "canvas.h"
#include "inputs.h"
#include "events.h"

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

        using KeyEvent = Event<Key, Renderer>;
        using KeyCharEvent = Event<Char, Renderer>;

        using MouseButtonEvent = Event<MouseButtonEventPayload, Renderer>;
        using MouseWheelEvent = Event<MouseWheelEventPayload, Renderer>;
        using MouseMoveEvent = Event<MouseMoveEventPayload, Renderer>;

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

        /** Root widget for modal interaction. Root element by default, but can be changed to another widget to limit the range of widgets that can receive mouse or keyboard events to a certain subtree. */ 
        Widget * modalRoot_ = nullptr;

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

    // ============================================================================================
    /** \name Keyboard Input.
     
        - update internal state
        - call own event (does not block by default) - can deal with global shortcuts
        - call target's implementation, if not blocked

        - target updates own internal state (and decides whether to block or not)
        - calls own event (does not block by default) 
        - if not blocked, calls on parent
        
     */
    //@{
    public:

        KeyEvent onKeyDown;
        KeyEvent onKeyUp;
        KeyCharEvent onKeyChar;

        /** Returns the widget that currently holds the keyboard focus. 

            nullptr when no widget is focused.  
         */
        Widget * keyboardFocus() const {
            return keyboardFocus_;
        }

    protected:

        /** Registers the key down event. 
         
            If key pressed is a modifier key, its modifier must be set on the key as well. 
         */
        virtual void keyDown(Key k);

        /** Registers key up event. 
         
            If we have a modifier key, then its modifier key must be already cleared. 
         */
        virtual void keyUp(Key k);

        virtual void keyChar(Char c);

        /** Returns the cutrrently active modifiers. 
         */
        Key modifiers() const {
            return modifiers_;
        }

        /** Updates the modifiers value without triggering the key down or up events. 
         
            This method only exists because renderers may have issues with getting the modifier states correctly (i.e. via classic terminal) when keys are pressed (i.e. on mouse move), in which case the modifiers should be updated, but the key events should not be emited because the key pressed happened at a different time
         */
        void setModifiers(Key value) {
            modifiers_ = value;
        }

    private:

        /** Widget that holds keyboard focus. */
        Widget * keyboardFocus_ = nullptr;
        Widget * keyDownFocus_ = nullptr;
        /** Current state of the modifier keys */
        Key modifiers_;

    //@}

    // ============================================================================================
    /** \name Mouse Input.
     */
    //@{
    public:
        MouseMoveEvent onMouseMove;
        MouseWheelEvent onMouseWheel;
        MouseButtonEvent onMouseDown;
        MouseButtonEvent onMouseUp;
        MouseButtonEvent onMouseClick;
        MouseButtonEvent onMouseDoubleClick;

    protected:

        virtual void mouseMove(Point coords);

        virtual void mouseWheel(Point coords, int by);

        virtual void mouseDown(Point coords, MouseButton button);

        virtual void mouseUp(Point coords, MouseButton button);

        virtual void mouseClick(Point coords, MouseButton button);

        virtual void mouseDoubleClick(Point coords, MouseButton button);

    private:

        void updateMouseFocus(Point coords);

        Widget * mouseFocus_ = nullptr;
        /** Mouse buttons that are currently down. */
        unsigned mouseButtons_ = 0;

    //@}

    }; // ui::Renderer

} // namespace ui
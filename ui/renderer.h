#pragma once

#include <deque>

#include "helpers/helpers.h"
#include "helpers/locks.h"
#include "helpers/time.h"

#include "canvas.h"
#include "inputs.h"
#include "events.h"
#include "event_queue.h"


namespace ui {

    class Widget;

    // ============================================================================================


    // ============================================================================================


    /** Base class for all UI renderer implementations. 
     
     */ 
    class Renderer {
        friend class Canvas;
        friend class Widget;
        friend class SelectionOwner;
    public:

        using VoidEvent = Event<void, Renderer>;

        using KeyEvent = Event<Key, Renderer>;
        using KeyCharEvent = Event<Char, Renderer>;

        using MouseButtonEvent = Event<MouseButtonEventPayload, Renderer>;
        using MouseWheelEvent = Event<MouseWheelEventPayload, Renderer>;
        using MouseMoveEvent = Event<MouseMoveEventPayload, Renderer>;

        using PasteEvent = Event<RendererPasteEventPayload, Renderer>;

        using Buffer = Canvas::Buffer;
        using Cell = Canvas::Cell;

        virtual ~Renderer();

    protected:

        Renderer(Size const & size, EventQueue & eq);

        Renderer(std::pair<int, int> const & size, EventQueue & eq):
            Renderer(Size{size.first, size.second}, eq) {
        }

    // ============================================================================================
    /** \name Events & Scheduling

        The renderer provides interface to schedule functions to be executed in trhe main UI thread. These functions can be tied to a particular widget belonging to the renderer, in which case the scheduled function will only execute igf the widget has not been detached in the meantime. If scheduled function is not tied to a widget, it will always execute as long as the renderer which created it still exists. 

        Internally each renderer has a dummy widget that is used to tie all its unregistered events, and which gets deleted when the renderer is deleted so that the events can be tracked. A widget remembers its number of currently scheduled events and if this is non-zero and the widget is detached, the event queue is examined and the events are descheduled. 

        The renderer and its event queue are decoupled so that multiple renderer instances running in same thread can share same event queue. The event queue also abstracts of the implementation details of scheduling and executing the events and allows event processing separate from the renderer itself. 
      */
    //@{
    public:

        /** Schedule the given event in the main UI thread.
         
            The event is bound to the specified widget that should be part of the widget tree attached to the renderer. If the widget is detached before the event is processed, the event is cancelled. 

            This function can be called from any thread as long as it does not clash with the destructor of the renderer. 
         */
        virtual void schedule(std::function<void()> event, Widget * widget) {
            eq_.schedule(event, widget);
        }

        /** Schedules the given event in the main UI thread. 
         
            The event is attached to no user widget and will only be cancelled if the renderer itself gets deleted before the event is processed. 

            This function can be called from any thread as long as it does not clash with the destructor of the renderer. 
         */
        void schedule(std::function<void()> event) {
            schedule(event, eventDummy_);
        }

        /** Yields to the UI thread. 
         
            This function can be called from any thread. Schedules an event in the UI thread and pauses the current thread until the UI thread has processed the event. 
         */ 
        void yieldToUIThread();

    protected:

        /** The queue for the events. */
        EventQueue & eq_;

    private:

        std::mutex yieldGuard_;
        std::condition_variable yieldCv_;

        /** A dummy widget the renderer can use to schedule renderer-specific events, such as repaints. */
        Widget * eventDummy_;

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

        Widget * modalRoot() const {
            return modalRoot_;
        }

        virtual void setModalRoot(Widget * widget); 

        /** Returns true if the renderer is currently showing a modal widget. 
         */
        bool isModal() const {
            return modalRoot_ != root_;
        }

    private:
        /** Triggered by the widgets when they are detached from the renderer. 
         
            When this method is called, the widget is guaranteed to be a part of valid tree (i.e. all its parents and children, but possibly not siblings are still attached). The bookkeeping should trigger any outstanding events on the widget and make sure that any references the renderer had to the detaching widget are voided.
         */
        virtual void widgetDetached(Widget * widget);

        /** Detaches the subtree of given widget from the renderer. 
         */
        void detachTree(Widget * root) {
            detachWidget(root);
            // now that the whole tree has been detached, fix keyboard focus, etc., special case is whet we are removing root_ widget, then there is no widget to set focus to, but the root is still attached (root_ is set to null only after it is removed)
            if (keyboardFocus_ == nullptr && root != root_)
                setKeyboardFocus(nextKeyboardFocus());
            // TODO do we need to fix mouse too? 
        }

        /** Detaches the given widget by invalidating its visible area, including its entire subtree, when detached, calls the widgetDetached() method.
         */
        void detachWidget(Widget * widget);

        Widget * root_ = nullptr;

        /** Root widget for modal interaction. Root element by default, but can be changed to another widget to limit the range of widgets that can receive mouse or keyboard events to a certain subtree. */ 
        Widget * modalRoot_ = nullptr;

        /** Backup for the non-modal keyboard focus element so that when modal root is returned, proper widget will be focused. */
        Widget * nonModalFocus_ = nullptr;

    //@}

    // ============================================================================================

    /** \name Layouting and Painting
     */
    //@{
    public:

        Size const & size() const {
            return buffer_.size();
        }

        int width() const {
            return buffer_.size().width();
        }

        int height() const {
            return buffer_.size().height();
        }

        /** Triggers repaint of the entire buffer. 

            Can be called from any thread as long as no overlap with destructor is possible. 
         */
        void repaint() {
            schedule([this](){
                if (root_ != nullptr)
                    paint(root_);
            });
        }

    protected: 
        /** Actual rendering. 
         */
        virtual void render(Rect const & rect) = 0;

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
                startFPSThread(); 
            } else {
                fps_ = value;
            }
        }

        /** Returns the visible area of the entire renderer. 
         */
        Canvas::VisibleArea visibleArea() {
            return Canvas::VisibleArea{Point{0,0}, Rect{buffer_.size()}};
        }

        /** Returns the paint buffer. 
         */
        Buffer const & buffer() const {
            return buffer_;
        }

    private:

        /** Instructs the renderer to repaint given widget. 
         
            Depending on the current fps settings the method either immediately repaints the given widget and initiates the rendering, or schedules the widget for rendering at next redraw. If there is already a widget scheduled for rendering, the scheduled widget is updated to be the common parent of the already requested and the newly requested widget. 
          */
        void paint(Widget * widget);

        /** Paints the scheduled widget on the renderer's buffer and calls the render() method immediately. 
         
            This method is either called by the fps renderer thread (if fps != 0), or by the paint() method and is responsible for actually repainting the scheduled widget. 
         */
        void paintAndRender();

        /** Starts the FPS thread. 
         
            The thread periodically invokes the paintAndRender() method based on the fps value. If fps is 0, the thread is stopped. To start the thread, fps must be set to value > 0. 
         */
        void startFPSThread();

        Buffer buffer_;
        Widget * renderWidget_{nullptr};
        std::atomic<unsigned> fps_{0};
        std::thread fpsThread_;

    //@}

    // ============================================================================================
    /** \name Keyboard Input.

        When a keyboard action is delivered to the renderer using the framework used, these actions must be translated by the subclasses to the methods below and called appropriately. 

        The renderer delegates keyboard events to the widget currently focused in the widget tree, if such widget exists. 
        - update internal state
        - call own event (does not block by default) - can deal with global shortcuts
        - call target's implementation, if not blocked

        - target updates own internal state (and decides whether to block or not)
        - calls own event (does not block by default) 
        - if not blocked, calls on parent
        
     */
    //@{
    public:

        VoidEvent onFocusIn;
        VoidEvent onFocusOut;

        KeyEvent onKeyDown;
        KeyEvent onKeyUp;
        KeyCharEvent onKeyChar;

        /** Returns the widget that currently holds the keyboard focus. 

            nullptr when no widget is focused.  
         */
        Widget * keyboardFocus() const {
            return focusIn_ ? keyboardFocus_ : nullptr;
        }

        virtual void setKeyboardFocus(Widget * widget); 

        Widget * nextKeyboardFocus();

        Widget * prevKeyboardFocus();

        /** Returns true if the renderer itself is focused from the UI point of view.
         
            This method returns true if the renderer is actually focused from the point of view of the UI framework used to renderer the widgets. Since the widgets do not correspond to the underlying primitives of the actual renderer, they cannot really grab proper focus, but instead the renderer delegates its own focus messages to them. 
         */
        bool rendererFocused() const {
            return focusIn_;
        }

    protected:

        /** The renderer's window has been focused. 
         
            This method must be called before any other keyboard input takes place. 
         */
        virtual void focusIn();

        /** The renderer's window has lost focus. 
         
            No keyboard input can be directed to the renderer after this method call unless focusIn() is not called first. 
         */
        virtual void focusOut();

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

        bool focusIn_ = false;

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
        VoidEvent onMouseIn;
        VoidEvent onMouseOut;
        MouseMoveEvent onMouseMove;
        MouseWheelEvent onMouseWheel;
        MouseButtonEvent onMouseDown;
        MouseButtonEvent onMouseUp;
        MouseButtonEvent onMouseClick;
        MouseButtonEvent onMouseDoubleClick;

        /** Returns true if the mouse is captured by the renderer from the UI's point of view. 
         
            Similarly to the keyboard focus, the renderer acts as a single blackbox widget for the frameworkl it renders itself on and this method returns whether from the UI's point of view the renderer captures the mouse.
         */
        bool rendererMouseCaptured() const {
            return mouseIn_;
        }

        /** Returns the widget that has active mouse capture. 
         */
        Widget * mouseFocus() const {
            return mouseIn_ ? mouseFocus_ : nullptr;
        }

    protected:

        /** Sets mouse cursor for the renderer.
         
            Called by widget's mouseIn and mouse cursor setters.
         */
        virtual void setMouseCursor(MouseCursor cursor) = 0;

        /** Mouse enters the renderer's area.

            Before any other mouse event happens, the mouseIn() method must be caled.
         */
        virtual void mouseIn();

        /** Mouse leaves the renderer's area.
         
            No further mouse events can happen after a call to mouseOut() unless mouseIn() is called first. 
         */
        virtual void mouseOut();

        virtual void mouseMove(Point coords);

        virtual void mouseWheel(Point coords, int by);

        virtual void mouseDown(Point coords, MouseButton button);

        virtual void mouseUp(Point coords, MouseButton button);

        virtual void mouseClick(Point coords, MouseButton button);

        virtual void mouseDoubleClick(Point coords, MouseButton button);

    private:

        void updateMouseFocus(Point coords);

        bool mouseIn_ = false;

        Widget * mouseFocus_ = nullptr;

        /** Mouse buttons that are currently down. */
        unsigned mouseButtons_ = 0;

        unsigned mouseClickButton_ = 0;
        unsigned lastMouseClickButton_ = 0;
        size_t mouseClickStart_ = 0;
        size_t lastMouseClickEnd_ = 0;
        Widget * lastMouseClickTarget_ = nullptr;

        size_t mouseClickMaxDuration_ = 200;
        size_t mouseDoubleClickMaxDistance_ = 200;

    //@}

    // ============================================================================================
    /** \name Selection & Clipboard support.
     
       Within single application, the renderer must support 
     */
    //@{
    public:

        PasteEvent onPaste;
    protected:

        Widget * selectionOwner() const {
            return selectionOwner_;
        }

        /** Sets the clipboard contents. 
         */ 
        virtual void setClipboard(std::string const & contents) = 0;

        /** Sets the selection contents and registers the owner. 
         */
        virtual void setSelection(std::string const & contents, Widget * owner) = 0;

        virtual void requestClipboard(Widget * sender) {
            clipboardRequestTarget_ = sender;
        }

        virtual void requestSelection(Widget * sender) {
            selectionRequestTarget_ = sender;
        }

        virtual void pasteClipboard(std::string const & contents);

        virtual void pasteSelection(std::string const & contents);

        /** Clears the selection owned by a widget associated with the renderer. 
         
            The method should either be called by the selection owning widget if it its selection has been clared (in which case the sender should be the requesting widget), or by the renderer itself, in which case the sender is nullptr. 

            When sender is nullptr, the current selection owner is notified by calling its Widget::clearSelection() method. 
         */
        virtual void clearSelection(Widget * sender);

    private:
        /** The widget owning the selection, nullptr if none. 
         */
        Widget * selectionOwner_ = nullptr;
        Widget * clipboardRequestTarget_ = nullptr;
        Widget * selectionRequestTarget_ = nullptr;

    //@}

    }; // ui::Renderer

} // namespace ui
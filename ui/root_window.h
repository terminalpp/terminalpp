#pragma once

#include <map>

#include "container.h"

namespace ui {

    class Renderer;
    class Panel;

    /**  */
    class RootWindow : public Container {
    public:

        /** Determines the icon of the renderer's window where appropriate. 
         
            Instead of specifying the actual icon, which is left to the actual renderers being used, the icon specifies the meaning of the icon.
         */
        enum class Icon {
            Default,
            Notification,
        }; // RootWindow::Icon

        using Widget::setFocused;
        using Container::setLayout;
        using Widget::repaint;
        using Widget::resize;

		RootWindow(int width = 0, int height = 0);

        ~RootWindow() override;

        std::string const & title() const {
            return title_;
        }

        Icon icon() const {
            return icon_;
        }

        /** Returns the background color of the window. 
         */
        Color backgroundColor() const {
            return backgroundColor_;
        }

        void setBackgroundColor(Color value) {
            if (backgroundColor_ != value) {
                backgroundColor_ = value;
                repaint();
            }
        }

        using Container::attachChild;
        using Container::detachChild;

        void showModalWidget(Widget * w, Widget * keyboardFocus, Layout * layout = Layout::Maximized);

        /** Returns true if the root window is currently covered with a modal widget. 
         */
        bool isModal() const {
            return modalWidgetActive_;
        }

        void hideModalWidget();

    protected:

        friend class Widget;
        
        Event<void> onMouseIn;
        Event<void> onMouseOut;
        Event<void> onWindowFocusIn;
        Event<void> onWindowFocusOut;
        
        // Interface to the renderer (and widget implementation where used by both)

        virtual void render(Rect const & rect);

        /** Called when new renderer is attached to the window. 
         */
        virtual void attachRenderer(Renderer * renderer) {
            ASSERT(renderer_ == nullptr);
            renderer_ = renderer;
        }

        virtual void rendererResized(int width, int height) {
            resize(width, height);
        }

        /** Locks the backing buffer and returns it in a RAII smart pointer.
         */
        Canvas::Buffer::Ptr buffer(bool priority = false) {
            if (priority)
                buffer_.priorityLock();
            else 
                buffer_.lock();
            return Canvas::Buffer::Ptr(buffer_, false);
        }

        /** Returns the cursor information, i.e. where & how to draw the cursor.
         */
        Cursor const & cursor() const {
            return cursor_;
        }

        /** Triggered when mouse leaves the are of the renderer window. 
         
            This is not identical to the window loosing focus. 
         */
        void inputMouseOut() {
            ASSERT(mouseFocus_ != nullptr);
            if (mouseCaptured_ == 0) {
                mouseIn_ = true; // temporarily make sure that mouseLeave will assume the mouse is still focused 
                mouseFocus_->mouseLeave();
                mouseIn_ = false;
                mouseFocus_ = this;
                mouseOut();
            } else {
                mouseIn_ = false;
            }
        }

        void inputSetFocus(bool value) {
            ASSERT(windowFocused_ != value);
            windowFocused_ = value;
            if (value)
                onWindowFocusIn(this);
            else
                onWindowFocusOut(this);
        }

        void mouseDown(int col, int row, MouseButton button, Key modifiers) override;
        void mouseUp(int col, int row, MouseButton button, Key modifiers) override;
        void mouseWheel(int col, int row, int by, Key modifiers) override;
        void mouseMove(int col, int row, Key modifiers) override;

        virtual void mouseIn() {
            // do nothing - we will receive mouse move event too, but other may want to override
            onMouseIn(this);
        }

        virtual void mouseOut() {
            onMouseOut(this);
            //checkMouseOverAndOut(nullptr);
        }

        /** Updates the mouse state when a mouse event occurs and recalculates the coordinates. 
         
            Dependning on the mouse capture and previous mouse position and focus widget emits the proper mouseEenter, mouseLeave and mouseIn events. 
            Also recalculates the coordinates given to it to the coordinates relative the the origin of the widget which has currently the mouse focus, i.e. over which the mouse currently hovers, or which has captured it. 
         */
        void updateMouseState(int & col, int & row);

        void keyChar(helpers::Char c) override;
        void keyDown(Key k) override;
        void keyUp(Key k) override;

        /** Notifies the root window that a widget has been attached. 
         */
        virtual void widgetAttached(Widget * widget) {
            if (widget->focusStop())
                addFocusStop(widget);
        }

        /** Notifies the root window that a widget has been detached within it. 
         
            The root window must then make sure that any of its internal mentions (such as keyboard or mouse focus and selection ownership) are cleared properly. 
         */
        virtual void widgetDetached(Widget * widget) {
            if (widget->focusStop_)
                removeFocusStop(widget);
            if (keyboardFocus_ == widget)
                focusWidget(widget, false);
            if (lastMouseTarget_ == widget)
                lastMouseTarget_ = nullptr;
            if (mouseFocus_ == widget) {
                mouseFocus_->mouseLeave();
                mouseFocus_ = this;
                mouseFocus_->mouseEnter();
            }
            if (mouseClickTarget_ == widget)
                mouseClickTarget_ = nullptr;
            if (pasteRequestTarget_ == widget)
                pasteRequestTarget_ = nullptr;
            if (selectionOwner_ == widget)
                clearSelection();
        }

        void invalidateContents() override;

        void updateSize(int width, int height) override;

        /** Called by widget that wants to obtain keyboard focus. 
         */
        bool focusWidget(Widget * widget, bool value);

        /** Focuses next element in the explicit focus stops ring. 
         
            If no element is currently focused, or if the currently focused element does not belong to the focus stop ring, focuses the first element from the ring, otherwise focuses the next element from the focus ring, or the first one, if the currently focused element is the end of the ring. 

            If the focus ring is empty, does nothing.
         */ 
        Widget * focusNext();

        /** Adds new focus stop widget. 
          
            A helper method called when existing child updates its stop status to true, or when new child that is a focus stop has been attached. 
         */
        void addFocusStop(Widget * child) {
            ASSERT(focusStops_.find(child->focusIndex_) == focusStops_.end()) << "Multiple focus index " << child->focusIndex();
            focusStops_.insert(std::make_pair(child->focusIndex(), child));
        }

        /** Removes the focus stop widget. 
         
            A helper method called when child widget stops being the focus stop, or when a focus stop child is detached. 
         */
        void removeFocusStop(Widget * child) {
            focusStops_.erase(child->focusIndex());
        }

		/** Takes screen coordinates and converts them to the widget's coordinates.

			Note that these may be negative, or bigger than the widget itself if the mouse is outside of the widget's area and the mouse target is locked.
		 */
		void screenToWidgetCoordinates(Widget const * w, int & col, int & row) {
			Canvas::VisibleRect const& wr = w->visibleRect_;
            ASSERT(wr.valid()) << "An invalid widget is rather bad at receiving mouse coordinates";
            wr.toWidgetCoordinates(col, row);
		} 

        Widget * getTransitiveMouseTarget(int & col, int & row) {
            Widget * result = this;
            while (true) {
                Widget * next = result->getMouseTarget(col, row);
                if (result == next)
                    return result;
                else
                    result = next;
            }
        }

        void setTitle(std::string const & title) {
            if (title_ != title) {
                updateTitle(title);
                // TODO on title change event
            }
        }

        void setIcon(Icon icon) {
            if (icon_ != icon) {
                updateIcon(icon);
                // TODO on icon change event
            }
        }

        /** Updates the mouse coordinates for the widget. 

            Does nothing, but stops the widget's search through the parent.  
         */
        void windowToWidgetCoordinates(int & col, int & row) override {
            MARK_AS_UNUSED(col);
            MARK_AS_UNUSED(row);
            ASSERT(x() == 0 && y() == 0 && parent() == nullptr);
        }        

        virtual void updateTitle(std::string const & title);

        virtual void updateIcon(Icon icon);

        virtual void closeRenderer();

        void paint(Canvas & canvas) override;


        virtual void paste(Widget * target, std::string const & contents) {
            target->paste(contents);
        }

    private:

        friend class Canvas;
        friend class Renderer;

        template<typename T>
        friend class SelectionOwner;

        // Clipboard & selection paste request and response

        /** Handles clipboard and selection paste requests. 
         */
        virtual void requestClipboardContents(Widget * sender);
        virtual void requestSelectionContents(Widget * sender);

        /** Triggered by the renderer when paste event is received. 
         
            Determines whether there is known widget that requested paste and if so, calls the paste(Widget*, std::string &) method that is responsible for actually delivering the contents of the keyboard to the target widget. 

            Clears the paste state. 
         */
        void paste(std::string const & contents) override;

        // Clipboard and selection updates

        /** Informs the renderer that clipboard contents should be updated to given text. 
         */
        virtual void setClipboard(std::string const & contents);

        /** Informs the renderer that selection has changed to given contents and owner. 
         */
        virtual void setSelection(Widget * owner, std::string const & contents);

        /** Called when ui side wants to clear selection it owns explicitly. 
         
            First the selection is invalidated on the UI side and then the renderer is informed that it should clear its selection information as well. 
         */
        virtual void clearSelection();

        /** Called by the renderer when the selection has been invalidated. 

            No need to inform the renderer, but selection owner has to be notified that its selection has been invalidated and the selection owner should be cleared.  
         */
        void selectionInvalidated() override;


        bool destroying_;

        /** Attached renderer. 
         */
        Renderer * renderer_;

        Canvas::Buffer buffer_;

        /** Holds whether the renderer window containing the root window is focused or not. 
         */
        bool windowFocused_;

        Widget * keyboardFocus_;
        std::map<unsigned, Widget *> focusStops_;

        /** Determins the last widget that was a mouse target so that we can determine when to generate mouseOver and mouseOut events. 
         */
        Widget * lastMouseTarget_;

        /** Mouse focus is locked when any button is down, and regardless of the movement, the same widget that had mouse focus when the first button was pressed down will receive the mouse events. 
         */
		Widget* mouseFocus_;

        /** Determines whether the mouse is inside the root window, or not. 
         */    
        bool mouseIn_;

        /** Determines the number of pressed buttons to know when the mouse focus lock is to be acquired and released. 
         */
        size_t mouseCaptured_;

        /** Duration in milliseconds between down & up events to pass as a mouse click. 
         */
        size_t mouseClickDuration_;

        /** Duration in milliseconds between two mouse click events to pass as a double click. 
         */
        size_t mouseDoubleClickDuration_;

		Widget* mouseClickTarget_;
		MouseButton mouseClickButton_;
		size_t mouseClickStart_;
		size_t mouseClickEnd_;

        Cursor cursor_;

        Widget * pasteRequestTarget_;
        Widget * selectionOwner_;

        std::string title_;
        Icon icon_;

        /* Default background of the window. 
         */
        Color backgroundColor_;

        /* The panel that displays the modal widget, if any. 
         */
        Panel * modalPane_;
        Widget * modalFocusBackup_;
        bool modalWidgetActive_;

    }; 


} // namespace ui
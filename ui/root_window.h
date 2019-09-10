#pragma once

#include "container.h"

namespace ui {

    class Clipboard;
    class Renderer;

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
        using Container::attachChild;
        using Widget::repaint;
        using Widget::resize;

		RootWindow();

        ~RootWindow() override {
            // one-way detach all children
            //for (Widget * child : children())
            //    child->detach
            // first set the destroy flag
            destroying_ = true;
            // then invalidate
            invalidate();
            // obtain the buffer - sync with end of any pending paints
            buffer();
        }

        std::string const & title() const {
            return title_;
        }

        Icon icon() const {
            return icon_;
        }

        /** Returns the background color of the window. 
         */
        Color background() const {
            return background_;
        }

    protected:

        friend class Widget;
        
	    static constexpr unsigned MOUSE_CLICK_MAX_DURATION = 100;

	    static constexpr unsigned MOUSE_DOUBLE_CLICK_MAX_INTERVAL = 300;

        
        // Interface to the renderer (and widget implementation where used by both)

        virtual void render(Rect const & rect);

        virtual void rendererResized(int width, int height) {
            resize(width, height);
        }

        virtual void rendererFocused(bool value) {
            setFocused(value);
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

        void mouseDown(int col, int row, MouseButton button, Key modifiers) override;
        void mouseUp(int col, int row, MouseButton button, Key modifiers) override;
        void mouseWheel(int col, int row, int by, Key modifiers) override;
        void mouseMove(int col, int row, Key modifiers) override;

        void keyChar(helpers::Char c) override;
        void keyDown(Key k) override;
        void keyUp(Key k) override;


        // widget implementation specific to renderer

        void updateFocused(bool value) override {
            // first make sure the focus in/out 
            Container::updateFocused(value);
            // if there is keyboard focused widget, update its state as well
            if (keyboardFocus_ != nullptr)
                keyboardFocus_->updateFocused(value);
        }

        void invalidateContents() override;

        void updateSize(int width, int height) override {
            // resize the buffer first
            {
                std::lock_guard<Canvas::Buffer> g(buffer_);
                buffer_.resize(width, height);
            }
            Container::updateSize(width, height);
        }

        /** Called by widget that wants to obtain keyboard focus. 
         
            TODO what to do if widget is removed from the hierarchy while focused?
         */
        void focusWidget(Widget * widget, bool value) {
            ASSERT(widget != nullptr);
            // if the widget is not the root window, update its focus accordingly and set the focused
            if (widget != this) {
                if (focused()) {
                    ASSERT(keyboardFocus_ == widget || value);
                    if (keyboardFocus_ != nullptr) 
                        keyboardFocus_->updateFocused(false);
                    keyboardFocus_ = value ? widget : nullptr;
                    if (keyboardFocus_ != nullptr)
                        keyboardFocus_->updateFocused(true);
                } else {
                    keyboardFocus_ = value ? widget : nullptr;
                }
            // if the widget is root window, then just call own updateFocused method, which sets the focus of the root window and updates the focus of the active widget, if any
         } else {
                updateFocused(value);
            }
        }

		Widget* mouseFocusWidget(int col, int row);

		/** Takes screen coordinates and converts them to the widget's coordinates.

			Note that these may be negative, or bigger than the widget itself if the mouse is outside of the widget's area and the mouse target is locked.
		 */
		Point screenToWidgetCoordinates(Widget * w, unsigned col, unsigned row) {
			Canvas::VisibleRegion const& wr = w->visibleRegion_;
            ASSERT(wr.valid) << "An invalid widget is rather bad at receiving mouse coordinates";
			Point result(
				wr.region.left() + (col - wr.windowOffset.x),
				wr.region.top() + (row - wr.windowOffset.y)
			);
			return result;
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

        virtual void updateTitle(std::string const & title);

        virtual void updateIcon(Icon icon);

        virtual void closeRenderer();

    private:

        friend class Canvas;
        friend class Clipboard;
        friend class Renderer;

        virtual void requestClipboardPaste(Clipboard * sender);
        virtual void requestSelectionPaste(Clipboard * sender);
        virtual void setClipboard(Clipboard * sender, std::string const & contents);
        virtual void setSelection(Clipboard * sender, std::string const & contents);
        virtual void clearSelection(Clipboard * sender);

        virtual void paste(std::string const & contents);

        virtual void invalidateSelection();

        bool destroying_;

        /** Attached renderer. 
         */
        Renderer * renderer_;

        Canvas::Buffer buffer_;

        Widget * keyboardFocus_;

        unsigned mouseFocusLock_; 
		int mouseCol_;
		int mouseRow_;
		Widget* mouseFocus_;
		Widget* mouseClickWidget_;
		MouseButton mouseClickButton_;
		size_t mouseClickStart_;
		size_t mouseDoubleClickPrevious_;

        Cursor cursor_;

        Clipboard * pasteRequestTarget_;
        Clipboard * selectionOwner_;

        std::string title_;
        Icon icon_;

        /* Default background of the window. 
         */
        Color background_;

    }; 


} // namespace ui
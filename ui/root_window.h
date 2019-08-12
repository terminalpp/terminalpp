#pragma once

#include "container.h"
#include "renderer.h"

namespace ui {

    /**  */
    class RootWindow : public Container, public Renderable {
    public:

        using Widget::setFocus;
        using Container::setLayout;
        using Container::addChild;
        using Widget::repaint;

		RootWindow(int width, int height) :
            Widget{0, 0, width, height},
			Container{0, 0, width, height},
			buffer_{width, height},
            keyboardFocus_{nullptr},
            mouseFocus_{nullptr} {
            visibleRegion_ = Canvas::VisibleRegion{this};
		}


    protected:

        friend class Widget;
        friend class Clipboard;
        
	    static constexpr unsigned MOUSE_CLICK_MAX_DURATION = 100;

	    static constexpr unsigned MOUSE_DOUBLE_CLICK_MAX_INTERVAL = 300;

        
        // renderable (and widget where appropriate)

        void rendererResized(int width, int height) override {
            resize(width, height);
        }

        void rendererFocused(bool value) override {
            setFocus(value);
        }

        /** Locks the backing buffer and returns it in a RAII smart pointer.
         */
        Canvas::Buffer::Ptr buffer(bool priority = false) override {
            if (priority)
                buffer_.priorityLock();
            else 
                buffer_.lock();
            return Canvas::Buffer::Ptr(buffer_, false);
        }

        /** Returns the cursor information, i.e. where & how to draw the cursor.
         */
        Cursor const & cursor() const override {
            return cursor_;
        }

        void mouseDown(int col, int row, MouseButton button, Key modifiers) override;
        void mouseUp(int col, int row, MouseButton button, Key modifiers) override;
        void mouseWheel(int col, int row, int by, Key modifiers) override;
        void mouseMove(int col, int row, Key modifiers) override;

        void keyChar(helpers::Char c) override;
        void keyDown(Key k) override;
        void keyUp(Key k) override;

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

			Returns (-1;-1) if the screen coordinates are outside the current widget.
		 */
		Point screenToWidgetCoordinates(Widget * w, unsigned col, unsigned row) {
			Canvas::VisibleRegion const& wr = w->visibleRegion_;
			if (!wr.valid() || wr.windowOffset.x > static_cast<int>(col) || wr.windowOffset.y > static_cast<int>(row))
				return Point(-1, -1);
			Point result(
				wr.region.left() + (col - wr.windowOffset.x),
				wr.region.top() + (row - wr.windowOffset.y)
			);
			ASSERT(result.x >= 0 && result.y >= 0);
			if (result.x >= w->width() || result.y >= w->height())
				return Point(-1, -1);
			return result;
		}

    private:

        friend class Canvas;

        /** Attached renderer. 
         */
        Renderer * renderer_;

        Canvas::Buffer buffer_;

        Widget * keyboardFocus_;

		int mouseCol_;
		int mouseRow_;
		Widget* mouseFocus_;
		Widget* mouseClickWidget_;
		MouseButton mouseClickButton_;
		size_t mouseClickStart_;
		size_t mouseDoubleClickPrevious_;

        Cursor cursor_;



    }; 


} // namespace ui
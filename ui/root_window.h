#pragma once

#include "container.h"

namespace ui {

    /** Base class for user interface renderers.
     */
    class Renderer {
    public:
        virtual int cols() const = 0;
        virtual int rows() const = 0;

    }; // ui::Renderer

    /**  */
    class RootWindow : public Container {
    public:

        using Container::setLayout;
        using Container::addChild;
        using Widget::repaint;

		RootWindow(int width, int height) :
			Container{0, 0, width, height},
			buffer_{width, height},
            keyboardFocus_{nullptr},
            mouseFocus_{nullptr} {
            visibleRegion_ = Canvas::VisibleRegion{this};
		}

        // events 

        /** Triggered by the root window when given rectangle of its buffer has changed and should be repainted. 
         */
        Event<RectEvent> onRepaint;



        virtual void rendererAttached(Renderer * renderer) {
            MARK_AS_UNUSED(renderer);

        }

        virtual void rendererDetached(Renderer * renderer) {
            MARK_AS_UNUSED(renderer);
        }

        virtual void rendererResized(Renderer * renderer, int width, int height) {
            MARK_AS_UNUSED(renderer);
            resize(width, height);
        }

        void mouseDown(int col, int row, MouseButton button, Key modifiers) override;
        void mouseUp(int col, int row, MouseButton button, Key modifiers) override;
        void mouseWheel(int col, int row, int by, Key modifiers) override;
        void mouseMove(int col, int row, Key modifiers) override;
        void keyChar(helpers::Char c) override;
        void keyDown(Key k) override;
        void keyUp(Key k) override;

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

    protected:

        friend class Widget;
        
	    static constexpr unsigned MOUSE_CLICK_MAX_DURATION = 100;

	    static constexpr unsigned MOUSE_DOUBLE_CLICK_MAX_INTERVAL = 300;

        /** Triggers the onRepaint event. 
         
            Called by the canvas destructor when last (and therefore the largest) canvas object is destroyed after the lock is released. 
         */
        virtual void repaint(Rect rect) {
            trigger(onRepaint, rect);
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

        void focusWidget(Widget * widget) {
            if (keyboardFocus_ != nullptr)
                keyboardFocus_->updateFocused(false);
            keyboardFocus_ = widget;
            if (keyboardFocus_ != nullptr)
                keyboardFocus_->updateFocused(true);
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
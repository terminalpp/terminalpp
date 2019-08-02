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
			Container(0, 0, width, height),
			buffer_(width, height) {
            visibleRegion_ = Canvas::VisibleRegion(this);
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

        void mouseDown(int col, int row, MouseButton button, Key modifiers);
        void mouseUp(int col, int row, MouseButton button, Key modifiers);
        void mouseWheel(int col, int row, int by, Key modifiers);
        void mouseMove(int col, int row, Key modifiers);

        /** Locks the backing buffer and returns it in a RAII smart pointer.
         */
        Canvas::Buffer::Ptr buffer(bool priority = false) {
            if (priority)
                buffer_.priorityLock();
            else 
                buffer_.lock();
            return Canvas::Buffer::Ptr(buffer_, false);
        }

    protected:
        
	    static constexpr unsigned MOUSE_CLICK_MAX_DURATION = 100;

	    static constexpr unsigned MOUSE_DOUBLE_CLICK_MAX_INTERVAL = 300;

        /** Triggers the onRepaint event. 
         
            Called by the canvas destructor when last (and therefore the largest) canvas object is destroyed after the lock is released. 
         */
        virtual void repaint(Rect rect) {
            trigger(onRepaint, rect);
        }

        void invalidateContents() override;

        void updateSize(int width, int height) {
            // resize the buffer first
            {
                std::lock_guard<Canvas::Buffer> g(buffer_);
                buffer_.resize(width, height);
            }
            Container::updateSize(width, height);
        }
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

    }; 


} // namespace ui
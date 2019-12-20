#pragma once

#include "helpers/time.h"

namespace ui {

	template<typename T>
	class Scrollable {
	public:

		Scrollable(int width, int height) :
			scrollOffset_{0,0},
			clientSize_{width, height} {
		}

        Point clientSize() const {
            return clientSize_;
        }

		/** Returns the visible rectangle. 
		 */
		Point scrollOffset() const {
			return scrollOffset_;
		}

        /** Sets the scroll offset. 
         
            The scroll offset is first trimmed to the client size and rect, i.e. values less than 0 or the canvas size subtracted by the client rectangle are changed accordingly. 
            Returns true if the possibly updated scroll offset (i.e. new valid scroll offset) was different from existing scroll offset, i.e. whether the requested scroll was valid, or out of bounds (false).
         */
        bool setScrollOffset(Point offset) {
            Rect clientRect = static_cast<T*>(this)->childRect();
            offset.x = std::max(offset.x, 0);
            offset.x = std::min(offset.x, clientSize_.x - clientRect.width());
            offset.y = std::max(offset.y, 0);
            offset.y = std::min(offset.y, clientSize_.y - clientRect.height());
            if (scrollOffset_ == offset)
                return false;
            updateScrollOffset(offset);
            return true;
        }		

	protected:

		Canvas getChildrenCanvas(Canvas & canvas) {
			Canvas result{canvas, static_cast<T*>(this)->childRect()};
			result.updateRect(Rect::FromWH(clientSize_.x, clientSize_.y));
			result.scroll(scrollOffset_);
			return result;
        }	

        void setClientSize(Point const & size) {
            if (clientSize_ == size)
                return;
            updateClientSize(size);
        }

        virtual void updateClientSize(Point const & size) {
            clientSize_ = size;
            static_cast<T*>(this)->repaint();
        }

		virtual void updateScrollOffset(Point const & value) {
			scrollOffset_ = value;
			static_cast<T*>(this)->repaint();			
		}

        /** Returns the start and length of a vertical scrollbar. 
         */
        std::pair<int, int> verticalScrollbar(int length) {
            return ScrollBarDimensions(length, clientSize_.y, scrollOffset_.y);
        }

        /** Returns the start and length of a horizontal scrollbar. 
         */
        std::pair<int, int> horizontalScrollbar(int length) {
            return ScrollBarDimensions(length, clientSize_.x, scrollOffset_.x);
        }		

	private:

        static std::pair<int, int> ScrollBarDimensions(int length, int max, int offset) {
            int sliderSize = std::max(1, length * length / max);
            int sliderStart = (offset + length == max) ? (length - sliderSize) : (offset * length / max);
			// make sure that slider starts at the top only if we are really at the top
			if (sliderStart == 0 && offset != 0)
			    sliderStart = 1;
            // if the slider would go beyond the length, adjust the slider start
            if (sliderStart + sliderSize > length)
                sliderStart = length - sliderSize;
            return std::make_pair(sliderStart, sliderSize);
        }	

		Point scrollOffset_;

		Point clientSize_;

	}; // ui::Scrollable

    /** Autoscrolling trait for scrollable widgets.
     */
    template<typename T>
    class AutoScroller {

    protected:

        /** Creates the autoscroller.
         */
        AutoScroller():
            autoScrollIncrement_{0,0} {
            autoScrollTimer_.setInterval(50);
            autoScrollTimer_.setHandler([this]() {
                return autoScrollStep();
            });
        }

        virtual ~AutoScroller() { }

        /** Starts the autoscroll feature. 
         
            Each step, the scroll offset will be updated by the given step util it is either stopped, or reaches the scrolling limits. 
         */
        void startAutoScroll(Point const & step) {
            autoScrollTimer_.stop();
            autoScrollIncrement_ = step;
            autoScrollTimer_.start();
        }

        /** Stops the autoscroll feature if active. 
          
            Does nothing if the the autoscroll is already stopped. 
         */
        void stopAutoScroll() {
            autoScrollTimer_.stop();
        }

        /** Returns true if the autoscroll feature is currently active. 
         */
        bool autoScrollActive() const {
            return autoScrollTimer_.running();
        }

        /** A single step of the autoscroll feature. 
         */    
        virtual bool autoScrollStep() {
            T* self = static_cast<T*>(this);
            return self->setScrollOffset(self->scrollOffset() + autoScrollIncrement_);
        }

    private:
        Point autoScrollIncrement_;

        helpers::Timer autoScrollTimer_;

    }; // ui::Autoscroller

} // namespace ui
#pragma once

#include "helpers/time.h"

#include "shapes.h"

namespace ui {

    /** Scrollable widget. 
     
        Is a CRTP from Widget. 

     */
    template<typename T>
    class Scrollable {
    public:
        
        virtual ~Scrollable() {}

        /** Returns the size of the client canvas. 
         */
        Point const & clientSize() const {
            return clientSize_;
        }

        /** Returns the scroll offset of the client canvas in the clientRect. 
         */
        Point const & scrollOffset() const {
            return scrollOffset_;
        }

        /** Sets the scroll offset. 
         
            The scroll offset is first trimmed to the client size and rect, i.e. values less than 0 or the canvas size subtracted by the client rectangle are changed accordingly. 

            Returns true if the possibly updated scroll offset (i.e. new valid scroll offset) was different from existing scroll offset, i.e. whether the requested scroll was valid, or out of bounds (false).
         */
        bool setScrollOffset(Point offset) {
            Rect clientRect = static_cast<T*>(this)->clientRect();
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

        Scrollable(int width, int height):
            clientSize_{width, height},
            scrollOffset_{0, 0} {
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


        virtual void updateScrollOffset(Point const & size) {
            scrollOffset_ = size;
            static_cast<T*>(this)->repaint();
        }

        Canvas getClientCanvas(Canvas & canvas) {
            return Canvas(canvas).resize(clientSize_).scrollBy(scrollOffset_);
        }

        Point translateCoordinates(Point const & coords) {
            return coords + scrollOffset_;
        }

        /** Updates the coordinates of the widget based on the current scroll offset. 
         
            This method is to be called from the class inheriting from scrollable. 
         */
        void windowToWidgetCoordinates(int & col, int & row) {
            col += scrollOffset_.x;
            row += scrollOffset_.y;
        }

        /** Returns the start and length of a vertical scrollbar. 
         */
        std::pair<int, int> verticalScrollbar(int length) {
            return scrollBar(length, clientSize_.y, scrollOffset_.y);
        }

        /** Returns the start and length of a horizontal scrollbar. 
         */
        std::pair<int, int> horizontalScrollbar(int length) {
            return scrollBar(length, clientSize_.x, scrollOffset_.x);
        }

		/** The size of the client canvas on which the children of the widget will draw themselves. 
		 */
		Point clientSize_;

		/** The offset of the visible area (clientRect_) of the client canvas from the origin.
		 */
		Point scrollOffset_;

    private:
        std::pair<int, int> scrollBar(int length, int max, int offset) {
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

    }; // ui::Scrollable


    /** Autoscrollable widget.
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
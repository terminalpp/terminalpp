#pragma once

#include "helpers/time.h"

#include "../widget.h"

#include "trait_base.h"

namespace ui2 {

    /** Scrollable widgets trait. 
     
        Implements the basic data and functionality for scrolling. 

     */
	template<typename T>
	class Scrollable : public TraitBase<Scrollable, T> {
	public:

        int scrollWidth() const {
            return scrollWidth_;
        }

        int scrollHeight() const {
            return scrollHeight_;
        }

        Point scrollOffset() const {
            return scrollOffset_;
        }

    protected:

        Scrollable(int width, int height):
            scrollWidth_{width},
            scrollHeight_{height},
            scrollOffset_{0,0} {
        } 

        virtual void setScrollOffset(Point offset) {
            scrollOffset_ = offset;
            downcastThis()->repaint();
        }

        /** \name Incremental scrolling
         */
        //@{
        bool scrollBy(Point by) {
            return scrollBy(by, downcastThis()->width(), downcastThis()->height());
        }

        bool scrollBy(Point by, int visibleWidth, int visibleHeight) {
            Point offset = scrollOffset_ + by;
            Point adjusted = Point::MinCoordWise(
                Point::MaxCoordWise(Point{0,0}, offset), 
                Point{scrollWidth_ - visibleWidth, scrollHeight_ - visibleHeight}
            );
            setScrollOffset(adjusted);
            return adjusted == offset;
        }
        //@}

        virtual void setScrollWidth(int value) {
            scrollWidth_ = value;
        }

        virtual void setScrollHeight(int value) {
            scrollHeight_ = value;
        }

        Canvas getContentsCanvas(Canvas canvas) {
            return canvas.resize(scrollWidth_, scrollHeight_).offset(scrollOffset_);
        }

        void setRect(Rect const & value) {
            if (value.width() > scrollWidth_)
                scrollWidth_ = value.width();
            if (value.height() > scrollHeight_)
                scrollHeight_ = value.height();
        }

    private:
        int scrollWidth_;
        int scrollHeight_;
        Point scrollOffset_;

    }; // ui::Scrollable

} // namespace ui

#ifdef HAHA

namespace uix {

	template<typename T>
	class Scrollable : public TraitBase<Scrollable, T> {
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
        using TraitBase<Scrollable, T>::downcastThis;

		Canvas getChildrenCanvas(Canvas & canvas) {
            return Canvas{canvas}.clip(downcastThis()->childRect()).resize(clientSize_).scrollBy(scrollOffset_);
        }	

        void setClientSize(Point const & size) {
            if (clientSize_ == size)
                return;
            updateClientSize(size);
        }

        virtual void updateClientSize(Point const & size) {
            clientSize_ = size;
            downcastThis()->repaint();
        }

		virtual void updateScrollOffset(Point const & value) {
			scrollOffset_ = value;
			downcastThis()->repaint();			
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
    class AutoScroller : public TraitBase<AutoScroller, T> {

    protected:
        using TraitBase<AutoScroller, T>::downcastThis;

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
            T* self = downcastThis();
            return self->setScrollOffset(self->scrollOffset() + autoScrollIncrement_);
        }

    private:
        Point autoScrollIncrement_;

        helpers::Timer autoScrollTimer_;

    }; // ui::Autoscroller

} // namespace ui

#endif
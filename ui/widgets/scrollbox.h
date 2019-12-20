#pragma once

#include "helpers/time.h"

#include "../container.h"

namespace ui {

	/** A container which allows its contents to be scrolled. 

	    ScrollBox explicitly specifies its client width and height as well as scroll offset and these are properly reported in the widget's rectangles. This has the effect of the widget's children being properly scrolled so that the top-left corner of child canvas corresponds to the client canvas at the scroll offset. 

		## Autoscrolling

		The ScrollBox provides a configurable auto scrolling feature which can be started by calling setAutoScroll() method and providing it with the desired one step increment. A new thread will be spawned which would periodically scroll the widget until the end in any direction. If the function is called with empty increment, the auto scrolling is stopped. The interval between auto scrolling steps can be set via the setAutoScrollDelay() method. The autoScrollStep() protected method can be overriden if the child requires an action to be taken at each auto scroll step. 

		## Scrollbars

		The scrollbox itself has no provision for scrollbar widgets, although these can be added externally using appropriate layouts and paired with the scrollbox. 

		The scrollbox however provides methods for drawing simple scrollbar overlays using the cell border attributes and various other helper methods such as the sliderPlacement() method for determining proper slider dimension and placement. 

		> Note that at the moment, use of scrollbar widgets taking up space is discouraged and no such widgets are provided by the framework. 
	 */
	class ScrollBox : public Widget {
	public:

		ScrollBox() :
			scrollOffset_{0,0},
			clientSize_{width(), height()},
		    autoScrollIncrement_{0,0} {
			autoScrollTimer_.setInterval(50);
			autoScrollTimer_.setHandler([this]() {
				bool cont = scrollBy(autoScrollIncrement_);
				autoScrollStep();
				return cont;
			});
		}

		Rect clientRect() const override {
			return Rect::FromWH(clientSize_.x, clientSize_.y);
		}

		/** Returns the visible rectangle. 
		 */
		Point scrollOffset() const override {
			return scrollOffset_;
		}

	protected:

		/** Sets the scroll offset, i.e. the coordiates of the top-left corner of the visible area.
		 
		    Scrolling past the visible area of the widget is not supported and if the desired offset is outside these bounds it is clipped before setting and the function returns false. If the requested offset is a valid scroll offset, true is returned. 

			Note that regardless of the value returned, the scroll offset will be updated unless the clipped value would be the same. 
		 */
		bool setScrollOffset(Point offset) {
			Point max = scrollOffsetMax();
			bool result = true;
			if (offset.x < 0) {
				offset.x = 0;
				result = false;
			}  else if (offset.x > max.x) {
				offset.x = max.x;
				result = false;
			}
			if (offset.y < 0) {
				offset.y = 0;
				result = false;
			} else if (offset.y > max.y) {
				offset.y = max.y;
				result = false;
			}
			if (scrollOffset_ != offset)
				updateScrollOffset(offset);
			return result;
		}

		/** Scrolls the contents by the given vector. 
		 
		    This is just a shorthand call to setScrollOffset, so the same applies here - if the offset is outside proper bounds, it is clipped and false is returned, otherwise the function returns true. 
		 */
		bool scrollBy(Point const & by) {
			return setScrollOffset(scrollOffset_ + by);
		}

		void setClientArea(int clientWidth, int clientHeight) {
			if (clientSize_.x != clientWidth || clientSize_.y != clientHeight) {
				clientSize_.x = clientWidth;
				clientSize_.y = clientHeight;
			}
		}

		virtual void updateScrollOffset(Point const & value) {
			scrollOffset_ = value;
			repaint();
		}

		void updateSize(int width, int height) override {
		    setClientArea(std::max(width, clientSize_.x), std::max(height, clientSize_.y));
		}

		int clientWidth() const {
			return clientSize_.x;
		}

		int clientHeight() const {
			return clientSize_.y;
		}

		/** Returns the maximum reasonable values for the scroll offset. 
		 */
		Point scrollOffsetMax() {
			Rect child = childRect();
			Rect client = clientRect();
			return Point(client.width() - child.width(), client.height() - child.height());
		}

		void setAutoScroll(Point increment) {
			if (increment != autoScrollIncrement_) {
				if (autoScrollIncrement_ == Point{0,0}) 
					autoScrollTimer_.start();
				else if (increment == Point{0, 0}) 
				    autoScrollTimer_.stop();
				autoScrollIncrement_ = increment;
			}
		}

		/** Returns true if the auto scroll feature is currently on. 
		 */
		bool autoScrollRunning() const {
			return autoScrollTimer_.running();
		}

		/** Returns the auto scroll step delay. 
		 */
		size_t autoScrollDelay() const {
			return autoScrollTimer_.interval();
		}

		/** Sets the interval of the auto scroller steps in milliseconds. 
		 */
		void setAutoScrollDelay(size_t value) {
			autoScrollTimer_.setInterval(value);
		}

		/** Determines proper values for vertical auto scroll. 
		 
		    Expects mouse coordinates as input. If the mouse is above, or below the widget, autoscroll will be enabled. 
		 */
		void calculateVerticalAutoScroll(int col, int row) {
			MARK_AS_UNUSED(col);
            if (row < 0)
                setAutoScroll(ui::Point{0,-1});
            else if (row > height())
                setAutoScroll(ui::Point{0, 1});
			else
			    setAutoScroll(ui::Point{0,0});
		}

		/** Triggered when autoscrolling scrolls the widget. 
		 
		    Should be implemented in children if any action is required on the scroll step. 
		 */
		virtual void autoScrollStep() {
			// do nothing
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

        Point autoScrollIncrement_;
		helpers::Timer autoScrollTimer_;

	};


} // namespace ui
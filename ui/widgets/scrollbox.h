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
	class ScrollBox : public virtual Widget {
	public:

		ScrollBox() :
			scrollOffset_{0,0},
			clientWidth_(width()),
			clientHeight_(height()),
		    autoScrollIncrement_{0,0} {
			autoScrollTimer_.setInterval(50);
			autoScrollTimer_.setHandler([this]() {
				bool cont = scrollBy(autoScrollIncrement_);
				autoScrollStep();
				return cont;
			});
		}

		Rect clientRect() const override {
			return Rect{clientWidth_, clientHeight_};
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
			if (clientWidth_ != clientWidth || clientHeight_ != clientHeight) {
				clientWidth_ = clientWidth;
				clientHeight_ = clientHeight;
			}
		}

		virtual void updateScrollOffset(Point const & value) {
			scrollOffset_ = value;
			repaint();
		}

		void updateSize(int width, int height) override {
		    setClientArea(std::max(width, clientWidth_), std::max(height, clientHeight_));
		}

		int clientWidth() const {
			return clientWidth_;
		}

		int clientHeight() const {
			return clientHeight_;
		}

		/** Draws the vertical scrollbar overlay. 
		 */ 
		void drawVerticalScrollbarOverlay(Canvas & canvas, Color color, bool thick = false) {
			canvas.borderClear(Rect{canvas.width() -1, 0, canvas.width(), canvas.height()});
			// the right line
			canvas.borderLineRight(Point{canvas.width() - 1, 0}, canvas.height(), color, thick);
			// calculate the position of the slider
			std::pair<int, int> slider{sliderPlacement(canvas.height(), clientHeight_, scrollOffset_.y, canvas.height())};
			canvas.borderLineRight(Point{canvas.width() - 1, slider.first}, slider.second, color, true);
			if (thick)
				canvas.borderLineLeft(Point{canvas.width() - 1, slider.first}, slider.second, color, true);
		}

		/** Draws the horizontal scrollbar overlay. 
		 */ 
		void drawHorizontalScrollbarOverlay(Canvas & canvas, Color color, bool thick = false) {
			canvas.borderClear(Rect{0, canvas.height() - 1, canvas.width(), canvas.height()});
			// the right line
			canvas.borderLineBottom(Point{0, canvas.height() - 1}, canvas.width(), color, thick);
			// calculate the position of the slider
			std::pair<int, int> slider{sliderPlacement(canvas.width(), clientWidth_, scrollOffset_.x, canvas.width())};
			canvas.borderLineBottom(Point{slider.first, canvas.height() - 1}, slider.second, color, true);
			if (thick)
				canvas.borderLineTop(Point{slider.first, canvas.height() - 1}, slider.second, color, true);
		}

		std::pair<int, int> sliderPlacement(int maxWidth, int maxVal, int pos, int size) {
			int sliderSize = std::max(1, size * maxWidth / maxVal);
			int sliderStart = pos * maxWidth / maxVal;
			// make sure that slider starts at the top only if we are really at the top
			if (sliderStart == 0 && pos != 0)
			    sliderStart = 1;
			if (pos + size == maxVal)
			    sliderStart = maxWidth - sliderSize;
			else
			    sliderStart = std::min(sliderStart, maxWidth - sliderSize);
			return std::make_pair(sliderStart, sliderSize);
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

	private:

		Point scrollOffset_;

		int clientWidth_;
		int clientHeight_;

        Point autoScrollIncrement_;
		helpers::Timer autoScrollTimer_;

	};


} // namespace ui
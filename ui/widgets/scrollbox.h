#pragma once

#include "helpers/time.h"

#include "../container.h"

namespace ui {

	/** A container which allows its contents to be scrolled. 

	    TODO how would Scrollbars fit into this? The idea now is that a scroll box can have scrollbars as special properties, these will not be children in the proper sense, but extra widgets. However, this messes up the overlay detection in the layouts. 

		TODO should there be some non public scrollable widget like Container, etc? 
	 */
	class ScrollBox : public virtual Widget {
	public:

		ScrollBox() :
			scrollLeft_(0),
			scrollTop_(0),
			clientWidth_(width()),
			clientHeight_(height()),
			scrollbarInactiveColor_(Color::White().setAlpha(64)),
			scrollbarActiveColor_(Color::Red().setAlpha(128)),
			scrollbarActive_(false),
		    autoScrollIncrement_{0,0} {
			autoScrollTimer_.setInterval(50);
			autoScrollTimer_.setHandler([this]() {
				Point i{scrollLeft_, scrollTop_};
				i += autoScrollIncrement_;
				Point max = scrollOffsetMax();
				bool cont = true;
				if (i.x < 0) {
					i.x = 0;
					cont = false;
				}  else if (i.x > max.x) {
					i.x = max.x;
					cont = false;
				}
				if (i.y < 0) {
					i.y = 0;
					cont = false;
				} else if (i.y > max.y) {
					i.y = max.y;
					cont = false;
				}
				setScrollOffset(i);
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
			return Point{scrollLeft_, scrollTop_};
		}

	protected:

		/** Sets the scroll offset, i.e. the coordiates of the top-left corner of the visible area.
		 */
		void setScrollOffset(Point const & offset) {
			if (scrollLeft_ != offset.x || scrollTop_ != offset.y)
				updateScrollOffset(offset.x, offset.y);
		}

		void setClientArea(int clientWidth, int clientHeight) {
			if (clientWidth_ != clientWidth || clientHeight_ != clientHeight) {
				clientWidth_ = clientWidth;
				clientHeight_ = clientHeight;
			}
		}

		virtual void updateScrollOffset(int scrollLeft, int scrollTop) {
			scrollLeft_ = scrollLeft;
			scrollTop_ = scrollTop;
			repaint();
		}

		void mouseMove(int col, int row, Key modifiers) override {
			MARK_AS_UNUSED(row);
			MARK_AS_UNUSED(modifiers);
			bool x = col == width() - 1;
			if (x != scrollbarActive_) {
        	    scrollbarActive_ = x;
				repaint();
			}
		}

		void mouseOut() override {
			if (scrollbarActive_) {
				scrollbarActive_ = false;
				repaint();
			}
		}
		

		void updateSize(int width, int height) override {
		    setClientArea(std::max(width, clientWidth_), std::max(height, clientHeight_));
		}

		int scrollLeft() const {
			return scrollLeft_;
		}

		int scrollTop() const {
			return scrollTop_;
		}

		int clientWidth() const {
			return clientWidth_;
		}

		int clientHeight() const {
			return clientHeight_;
		}

		void scrollVertical(int diff) {
			int x = scrollTop_ + diff;
			x = std::max(x, 0);
			x = std::min(x, clientHeight_ - childRect().height());
			setScrollOffset(Point{scrollLeft_, x});			    
		}

		/** Draws the vertical scrollbar overlay. 
		 */ 
		void drawVerticalScrollbarOverlay(Canvas & canvas) {
			ui::Color color{scrollbarActive_ ? scrollbarActiveColor_ : scrollbarInactiveColor_};
			// the right line
			canvas.borderLineRight(Point{canvas.width() - 1, 0}, canvas.height(), color, false);
			// calculate the position of the slider
			std::pair<int, int> slider{sliderPlacement(canvas.height(), clientHeight_, scrollTop_, canvas.height())};
			canvas.borderLineRight(Point{canvas.width() - 1, slider.first}, slider.second, color, true);
		}

		/** Draws the horizontal scrollbar overlay. 
		 */ 
		void drawHorizontalScrollbarOverlay(Canvas & canvas) {
			ui::Color color{scrollbarActive_ ? scrollbarActiveColor_ : scrollbarInactiveColor_};
			// the right line
			canvas.borderLineBottom(Point{0, canvas.height() - 1}, canvas.width(), color, false);
			// calculate the position of the slider
			std::pair<int, int> slider{sliderPlacement(canvas.width(), clientWidth_, scrollLeft_, canvas.width())};
			canvas.borderLineBottom(Point{slider.first, canvas.height() - 1}, slider.second, color, true);
		}

		std::pair<int, int> sliderPlacement(int maxWidth, int maxVal, int pos, int size) {
			int sliderSize = std::max(1, size * maxWidth / maxVal);
			int sliderStart = pos * maxWidth / maxVal;
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
		 */
		virtual void autoScrollStep() {
			// do nothing
		}

	private:

		int scrollLeft_;
		int scrollTop_;

		int clientWidth_;
		int clientHeight_;

	    Color scrollbarInactiveColor_;
		Color scrollbarActiveColor_;

		bool scrollbarActive_;

        Point autoScrollIncrement_;
		helpers::Timer autoScrollTimer_;
		size_t autoScrollDelay_;

	};


} // namespace ui
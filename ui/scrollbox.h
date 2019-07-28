#pragma once

#include "container.h"

namespace ui {

	/** A container which allows its contents to be scrolled. 

	    TODO how would Scrollbars fit into this? The idea now is that a scroll box can have scrollbars as special properties, these will not be children in the proper sense, but extra widgets. However, this messes up the overlay detection in the layouts. 
	 */
	class ScrollBox : public Container {
	public:

		ScrollBox() :
			Container(),
			scrollWidth_(1),
			scrollHeight_(1),
			scrollLeft_(0),
			scrollTop_(0) {
		}

		using Container::addChild;
		using Container::setLayout;

		// TODO move these to proected?

		/** Sets the scroll size, i.e. the canvas of the widget whose portion is displayed in the client area.
		 */
		void setScrollSize(int width, int height) {
			if (scrollWidth_ != width || scrollHeight_ != height)
				updateScrollSize(width, height);
		}

		/** Sets the scroll offset, i.e. the coordiates of the top-left corner of the visible area.
		 */
		void setScrollOffset(int left, int top) {
			if (scrollLeft_ != left || scrollTop_ != top)
				updateScrollOffset(left, top);
		}

		virtual int scrollWidth() const {
			return scrollWidth_;
		}

		virtual int scrollHeight() const {
			return scrollHeight_;
		}

		int scrollLeft() const {
			return scrollLeft_;
		}

		int scrollTop() const {
			return scrollTop_;
		}

	protected:


		virtual void updateScrollSize(int scrollWidth, int scrollHeight) {
			scrollWidth_ = scrollWidth;
			scrollHeight_ = scrollHeight;
			// TODO how to do relayout nicely
			relayout_ = true;
			invalidate();
		}

		virtual void updateScrollOffset(int scrollLeft, int scrollTop) {
			scrollLeft_ = scrollLeft;
			scrollTop_ = scrollTop;
			invalidate();
		}

		Canvas getClientCanvas(Canvas& canvas) override {
			// create the canvas of appropriate size
			Canvas result(canvas, border().left, border().top, scrollWidth_, scrollHeight_);
			// first reposition the visible region according to positive scroll offsets
			result.visibleRegion_.region = Rect::Intersection(
				Rect(scrollWidth_, scrollHeight_),
				result.visibleRegion_.region + Point(std::max(0, scrollLeft_), std::max(0, scrollTop_))
			);
			// negative offsets update the window offset of the visible region and make the visible region smaller by their value
			if (scrollLeft_ < 0) {
				result.visibleRegion_.windowOffset.col -= scrollLeft_;
				result.visibleRegion_.region.right += scrollLeft_;
			}
			if (scrollTop_ < 0) {
				result.visibleRegion_.windowOffset.row -= scrollTop_;
				result.visibleRegion_.region.bottom += scrollTop_;
			}
			return result;
		}

	private:

		int scrollWidth_;
		int scrollHeight_;
		int scrollLeft_;
		int scrollTop_;

	};


} // namespace ui
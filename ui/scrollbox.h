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
			// TODO how to do relayout
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
			// offset the visible region by the scrollLeft and scrollRight
			result.visibleRegion_.region = Rect::Intersection(
				Rect(scrollWidth_, scrollHeight_),
				result.visibleRegion_.region + Point(scrollLeft_, scrollTop_)
			);
			return result;
		}

	private:

		int scrollWidth_;
		int scrollHeight_;
		int scrollLeft_;
		int scrollTop_;

	};


} // namespace ui
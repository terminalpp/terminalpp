#pragma once

#include "../container.h"

namespace ui {

	/** A container which allows its contents to be scrolled. 

	    TODO how would Scrollbars fit into this? The idea now is that a scroll box can have scrollbars as special properties, these will not be children in the proper sense, but extra widgets. However, this messes up the overlay detection in the layouts. 

		TODO should there be some non public scrollable widget like Container, etc? 
	 */
	class ScrollBox : public PublicContainer {
	public:

		ScrollBox(int x, int y, int width, int height) :
		    Widget{x, y, width, height},
			PublicContainer(x, y, width, height),
			scrollWidth_(1),
			scrollHeight_(1),
			scrollLeft_(0),
			scrollTop_(0) {
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

	protected:


		virtual void updateScrollSize(int scrollWidth, int scrollHeight) {
			scrollWidth_ = scrollWidth;
			scrollHeight_ = scrollHeight;
			scheduleRelayout();
			invalidate();
		}

		virtual void updateScrollOffset(int scrollLeft, int scrollTop) {
			scrollLeft_ = scrollLeft;
			scrollTop_ = scrollTop;
			invalidate();
		}

		Canvas getClientCanvas(Canvas& canvas) override {
			return Canvas(canvas, -scrollLeft_, -scrollTop_, scrollWidth_, scrollHeight_);
		}

	private:

		int scrollWidth_;
		int scrollHeight_;
		int scrollLeft_;
		int scrollTop_;

	};


} // namespace ui
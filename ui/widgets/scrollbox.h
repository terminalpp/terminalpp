#pragma once

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
			clientHeight_(height()) {
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

	private:

		int scrollLeft_;
		int scrollTop_;

		int clientWidth_;
		int clientHeight_;
	};


} // namespace ui
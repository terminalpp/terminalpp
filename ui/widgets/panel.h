#pragma once

#include "../container.h"

#include "../traits/box.h"

namespace ui {

	/** A simple container with custom background and a possibility of a border. 

	 */
	class Panel : public Container, public Box<Panel> {
	public:

		Panel(int width = 0, int height = 0) :
			Container(width, height),
			childMargin_{0, 0, 0, 0} {
		}

		Rect childRect() const override {
			return Rect::FromCorners(childMargin_.left, childMargin_.top, width() - childMargin_.right, height() - childMargin_.bottom);
		}

	protected:

		Margin const& childMargin() const {
			return childMargin_;
		}

		void setChildMargin(Margin const& value) {
			if (value != childMargin_) {
				updateChildMargin(value);
				scheduleRelayout();
				repaint();
			}
		}

		/** Updates the value of the widget's border and invalidates the widget.
		 */
		virtual void updateChildMargin(Margin const& value) {
			childMargin_ = value;
			invalidate();
		}

		void paint(Canvas & canvas) override {
			Box::paint(canvas);
			//canvas.fill(Rect::FromWH(width(), height()), background_);
			Container::paint(canvas);
		}

	private:

		/* Border of the panel */
		Margin childMargin_;

	}; // ui::Panel


} // namespace ui


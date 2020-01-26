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
			border_(0, 0, 0, 0) {
		}

/*
		Brush const & background() const {
			return background_;
		}

		void setBackground(Brush const & value) {
			if (background_ != value) {
				background_ = value;
				setForceOverlay(!background_.color.opaque());
                repaint();
			}
		}
		*/

		Rect childRect() const override {
			return Rect::FromCorners(border_.left, border_.top, width() - border_.right, height() - border_.bottom);
		}

	protected:

		Border const& border() const {
			return border_;
		}

		void setBorder(Border const& value) {
			if (value != border_) {
				updateBorder(value);
				scheduleRelayout();
				repaint();
			}
		}

		/** Updates the value of the widget's border and invalidates the widget.
		 */
		virtual void updateBorder(Border const& value) {
			border_ = value;
			invalidate();
		}

		void paint(Canvas & canvas) override {
			Box::paint(canvas);
			//canvas.fill(Rect::FromWH(width(), height()), background_);
			Container::paint(canvas);
		}

	private:

		/* Border of the panel */
		Border border_;

		//Brush background_;
 
	}; // ui::Panel


} // namespace ui


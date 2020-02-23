#pragma once

#include "../container.h"

#include "../traits/box.h"

namespace ui {

	/** A simple container with custom background and a possibility of a border. 

	 */
	class Panel : public Container, public Box<Panel> {
	public:

		Panel(int width = 0, int height = 0) :
			Container{width, height},
			showHeader_{false} {
		}

		Rect childRect() const override {
			if (showHeader_) 
				return Rect::FromCorners(0, headerFont_.height(), width(), height());
			else
				return Rect::FromCorners(0, 0, width(), height());
		}

	protected:

		void paint(Canvas & canvas) override {
			Box::paint(canvas);
			if (showHeader_) {
				canvas.setBg(headerBackground_).fill(Rect::FromTopLeftWH(0,0,canvas.width(), headerFont_.height()));
				canvas.setFg(headerColor_).setFont(headerFont_).lineOut(Point{0, 0}, headerText_);
			}
			Container::paint(canvas);
		}

	//private:

		// invalidate on change to font as childRect changes...
	    Font headerFont_;
		Color headerColor_;
		Brush headerBackground_;
		std::string headerText_;
		bool showHeader_;

	}; // ui::Panel


} // namespace ui


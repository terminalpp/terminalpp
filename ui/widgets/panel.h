#pragma once

#include "../container.h"

namespace ui {

	/** A simple container with custom background and a possibility of a border. 

	 */
	class Panel : public PublicContainer {
	public:

		Panel(int x, int y, int width, int height) :
		    Widget{x, y, width, height},
			PublicContainer(x, y, width, height),
			border_(0, 0, 0, 0) {
		}

		/** The clientWidth and clientHeight return the size of the widget that is available for children (i.e. excluding the border of the control.
		 */
		int clientWidth() const {
			return std::max(width() - border_.left - border_.right, 0);
		}

		int clientHeight() const {
			return std::max(height() - border_.top - border_.bottom, 0);
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

		/** Given a canvas for the full widget, returns a canvas for the client area only.
		 */
		Canvas getClientCanvas(Canvas& canvas) override{
			return Canvas(canvas, border_.left, border_.top, clientWidth(), clientHeight());
		}


		/** Updates the value of the widget's border and invalidates the widget.
		 */
		virtual void updateBorder(Border const& value) {
			border_ = value;
			invalidate();
		}

	private:

		/* Border of the panel */
		Border border_;

 
	}; // ui::Panel


} // namespace ui


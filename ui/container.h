#pragma once

#include <vector>

#include "control.h"

namespace ui {

	/** Container of controls. 
	 */
	class Container : public Control {
	public:


	protected:

		Container(Coord width, Coord height) :
			Control(width, height) {
		}

		/** Calls the paint method of all its children. 
		 */
		void doPaint(Canvas& canvas) override {
			canvas.fill(Rect(canvas.width(), canvas.height()), Color::Red(), Color::White(), 'x', Font());
		}

	private:
		std::vector<Control*> children_;


	};


} // namespace ui
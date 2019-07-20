#pragma once

#include <vector>

#include "control.h"

namespace ui {

	/** Container of controls. 
	 */
	class Container : public Control {
	public:

		/** Returns the layout used for children. 
		 */
		Layout * layout() const {
			return layout_;
		}

		/** Sets the layout used for children. 
		 */
		void setLayout(Layout * value) {
			if (layout_ != value) {
				layout_ = value;
				// TODO resize the children, figure out how
			}
		}

		Container(Control* parent, int left, int top, unsigned width, unsigned height) :
			Control(parent, left, top, width, height) {
		}

		/** Container deletes all its children as well. 
		 */
		~Container() {
			for (auto i : children_)
				delete i;
		}
	protected:

		Container(unsigned width, unsigned height) :
			Control(width, height),
		    layout_(Layout::None()) {
		}

		void doRegisterChild(Control* child) override {
			children_.push_back(child);
		}

		/** Calls the paint method of all its children. 
		 */
		void doPaint(Canvas& canvas) override {
			canvas.fill(Rect(canvas.width(), canvas.height()), Color::Red(), Color::White(), 'x', Font());
			for (Control* child : children_)
				doUpdateChild(canvas, child);
		}

	private:
		std::vector<Control*> children_;

		Layout * layout_;


	};


} // namespace ui
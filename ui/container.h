#pragma once

#include <vector>

#include "control.h"
#include "layout.h"

namespace ui {

	/** Container of controls. 
	 */
	class Container : public Control {
	public:

		Container(int left, int top, unsigned width, unsigned height) :
			Control(left, top, width, height),
		    layout_(Layout::None()) {
		}

		/** Container deletes all its children as well.
		 */
		~Container() override {
			for (auto i : children_)
				delete i;
		}

		void addChild(Control* child) {
			if (child->parent_ != nullptr) {
				Container* p = dynamic_cast<Container*>(child->parent_);
				if (p != nullptr)
					p->removeChild(child);
			}
			child->parent_ = this;
			children_.push_back(child);
			doChildGeometryChanged(child);
			repaint();
		}

		void removeChild(Control* child) {
			NOT_IMPLEMENTED;
		}

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

	protected:

		void doResize(unsigned width, unsigned height) override {
			width_ = width;
			height_ = height;
			layout_->resized(this);
		}

		/** Calls the paint method of all its children. 
		 */
		void doPaint(Canvas& canvas) override {
			canvas.fill(Rect(canvas.width(), canvas.height()), Color::Red(), Color::White(), 'x', Font());
			for (Control* child : children_)
				doUpdateChild(canvas, child);
		}

		/** When container's child geometry changes, the contrainer consults its layout to determine whether such update is allowed. 
	 	 */
		void doChildGeometryChanged(Control* child) override {
			layout_->childChanged(this, child);
		}

	private:

		friend class Layout;

		std::vector<Control*> children_;

		Layout * layout_;


	};


} // namespace ui
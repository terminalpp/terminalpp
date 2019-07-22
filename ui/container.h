#pragma once

#include <vector>

#include "widget.h"

namespace ui {

	class Container : public Widget {
	public:


		~Container() override {
			for (Widget* child : children_)
				delete child;
		}

	protected:

		Container(int x = 0, int y = 0, unsigned width = 1, unsigned height = 0) :
			Widget(x, y, width, height),
			relayout_(true) {
		}

		void addChild(Widget * child) {
			ASSERT(child->parent_ != this) << "Already a child";
			if (child->parent_ != nullptr) {
				Container* oldParent = dynamic_cast<Container*>(child->parent_);
				ASSERT(oldParent != nullptr) << "Moving only works between containers";
				oldParent->removeChild(child);
			}
			children_.push_back(child);
			child->parent_ = this;
			relayout_ = true;
			repaint();
		}

		Widget* removeChild(Widget* child) {
			ASSERT(child->parent_ == this) << "Not a child";
			child->parent_ = nullptr;
			for(auto i = children_.begin(), e = children_.end(); i != e; ++i)
				if (*i == child) {
					children_.erase(i);
					break;
				}
			relayout_ = true;
			repaint();
			return child;
		}

		void invalidateContents() override {
			Widget::invalidateContents();
			relayout_ = true;
			for (Widget* child : children_)
				child->invalidateContents();
		}

		void childInvalidated(Widget* child) override {
			relayout_ = true;
			Widget::childInvalidated(child);
		}

		void updatePosition(int x, int y) override {
			relayout_ = true;
			Widget::updatePosition(x, y);
		}

		void updateSize(unsigned width, unsigned height) override {
			relayout_ = true;
			Widget::updateSize(width, height);
		}

		/** The container assumes that its children will be responsible for painting the container itself in the paint method, while the container's paint method draws the children themselves. 
		 */
		void paint(Canvas& canvas) override {
			canvas.fill(Rect(canvas.width(), canvas.height()), Color::Red(), Color::White(), 'x', Font());
			if (relayout_) {
				// TODO deal with layout

				relayout_ = false;
			}
			// display the children
			for (Widget* child : children_)
				paintChild(child, canvas);
		}

	private:
		bool relayout_;

		std::vector<Widget*> children_;


	}; // ui::Container

} // namespace ui
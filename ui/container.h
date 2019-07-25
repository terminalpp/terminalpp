#pragma once

#include <vector>

#include "widget.h"

namespace ui {

	class Layout;

	/** Container is a widget capable of managing its children at runtime. 
	 */
	class Container : public Widget {
	public:

		~Container() override;

		Layout* layout() const {
			return layout_;
		}

	protected:

		Container(int x = 0, int y = 0, unsigned width = 1, unsigned height = 0);

		void addChild(Widget * child) {
			ASSERT(child->parent() != this) << "Already a child";
			if (child->parent() != nullptr) {
				Container* oldParent = dynamic_cast<Container*>(child->parent());
				ASSERT(oldParent != nullptr) << "Moving only works between containers";
				oldParent->removeChild(child);
			}
			children_.push_back(child);
			child->updateParent(this);
			relayout_ = true;
			repaint();
		}

		Widget* removeChild(Widget* child) {
			ASSERT(child->parent() == this) << "Not a child";
			child->updateParent(nullptr);
			for(auto i = children_.begin(), e = children_.end(); i != e; ++i)
				if (*i == child) {
					children_.erase(i);
					break;
				}
			relayout_ = true;
			repaint();
			return child;
		}

		void setLayout(Layout* value) {
			ASSERT(value != nullptr) << "use Layout::None instead";
			if (layout_ != value) {
				layout_ = value;
				relayout_ = true;
				repaint();
			}
		}

		virtual void setChildGeometry(Widget* child, int x, int y, unsigned width, unsigned height) {
			if (child->x() != x || child->y() != y) {
				if (child->visibleRegion_.isValid())
					child->invalidateContents();
				child->updatePosition(x, y);
			}
			if (child->width() != width || child->height() != height) {
				if (child->visibleRegion_.isValid())
				    child->invalidateContents();
				child->updateSize(width, height);
			}
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
		void paint(Canvas& canvas) override;

		void updateOverlay(bool value) override;

	private:

		friend class Layout;

		std::vector<Widget*> children_;

		Layout* layout_;
		bool relayout_;



	}; // ui::Container

} // namespace ui
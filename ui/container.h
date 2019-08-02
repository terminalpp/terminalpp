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

		Container(int x, int y, int width, int height);

		void addChild(Widget * child) {
			ASSERT(child->parent() != this) << "Already a child";
			if (child->parent() != nullptr) {
				Container* oldParent = dynamic_cast<Container*>(child->parent());
				ASSERT(oldParent != nullptr) << "Moving only works between containers";
				oldParent->removeChild(child);
			}
			children_.push_back(child);
			child->updateParent(this);
			scheduleRelayout();
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
			scheduleRelayout();
			repaint();
			return child;
		}

		void setLayout(Layout* value) {
			ASSERT(value != nullptr) << "use Layout::None instead";
			if (layout_ != value) {
				layout_ = value;
				scheduleRelayout();
				repaint();
			}
		}

		virtual void setChildGeometry(Widget* child, int x, int y, int width, int height) {
			if (child->x() != x || child->y() != y) {
				if (child->visibleRegion_.valid())
					child->invalidateContents();
				child->updatePosition(x, y);
			}
			if (child->width() != width || child->height() != height) {
				if (child->visibleRegion_.valid())
				    child->invalidateContents();
				child->updateSize(width, height);
			}
		}

		void invalidateContents() override {
			Widget::invalidateContents();
			scheduleRelayout();
			for (Widget* child : children_)
				child->invalidateContents();
		}

		void childInvalidated(Widget* child) override {
			scheduleRelayout();
			Widget::childInvalidated(child);
		}

		void updatePosition(int x, int y) override {
			scheduleRelayout();
			Widget::updatePosition(x, y);
		}

		void updateSize(int width, int height) override {
			scheduleRelayout();
			Widget::updateSize(width, height);
		}

		/** The container assumes that its children will be responsible for painting the container itself in the paint method, while the container's paint method draws the children themselves. 
		 */
		void paint(Canvas& canvas) override;

		void updateOverlay(bool value) override;

		Widget* getMouseTarget(unsigned col, unsigned row) override {
			ASSERT(visibleRegion_.contains(col, row));
			for (auto i = children_.rbegin(), e = children_.rend(); i != e; ++i)
				if ((*i)->visibleRegion_.contains(col, row))
					return (*i)->getMouseTarget(col, row);
			return this;
		}

		/** Schedules layout of all components on the next repaint event without actually triggering the repaint itself. 
		 */
		void scheduleRelayout() {
			relayout_ = true;
		}

	private:

		friend class Layout;

    	std::vector<Widget*> children_;

		Layout* layout_;
		bool relayout_;

	}; // ui::Container


	/** Exposes the container's interface publicly. 
	 */
	class PublicContainer : public Container {
	public:

		// Events from Widget

		using Widget::onShow;
		using Widget::onHide;
		using Widget::onResize;
		using Widget::onMove;
		using Widget::onMouseDown;
		using Widget::onMouseUp;
		using Widget::onMouseClick;
		using Widget::onMouseDoubleClick;
		using Widget::onMouseWheel;
		using Widget::onMouseMove;
		using Widget::onMouseEnter;
		using Widget::onMouseLeave;

		// Methods from Widget

		using Widget::setVisible;
		using Widget::move;
		using Widget::resize;
		using Widget::setWidthHint;
		using Widget::setHeightHint;

		// Methods from Container

		using Container::addChild;
		using Container::removeChild;
		using Container::setLayout;

		PublicContainer(int x = 0, int y = 0, int width = 1, int height = 1) :
			Container(x, y, width, height),
		    background_(Color::Black()) {
		}

		/** Returns the background of the container.
		 */
		Brush const& backrgound() const {
			return background_;
		}

		/** Sets the background of the container. 
		 */
		void setBackground(Brush const& value) {
			if (background_ != value) {
				background_ = value;
				setForceOverlay(!background_.color.opaque());
				repaint();
			}
		}

	protected:

		void paint(Canvas& canvas) override {
			canvas.fill(Rect(canvas.width(), canvas.height()), Brush(Color::Black()));
			Container::paint(canvas);
		}

	private:

		Brush background_;




	};

} // namespace ui
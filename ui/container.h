#pragma once

#include <vector>

#include "widget.h"

namespace ui {

	class Layout;

	/** Container is a widget capable of managing its children at runtime. 
	 */
	class Container : public Widget {
	public:

		Layout* layout() const {
			return layout_;
		}

	protected:
		friend class Widget;
		friend class RootWindow;

		Container(int width = 0, int height = 0);

        std::vector<Widget *> const & children() const {
            return children_;
        }

        std::vector<Widget *> & children() {
            return children_;
        }

        void attachChild(Widget * child) {
			if (child->parent_ == this) {
			    auto i = std::find(children_.begin(), children_.end(), child);
				std::swap(*i, children_.back());
			} else {
				// first detach the child if it is attached to other widget
				if (child->parent_ != nullptr)
					child->parent_->detachChild(child);
                children_.push_back(child);
                child->parent_ = this;
			}
			if (rootWindow() != nullptr)
			    child->attachRootWindow(rootWindow());
            scheduleRelayout();
            repaint();
        }

        void detachChild(Widget * child) {
            ASSERT(child->parent_ == this);
			// so that anything from the child won't go to parent
			child->parent_ = nullptr;
			// so that anything from the parent won't go to the child
		    children_.erase(std::find(children_.begin(), children_.end(), child));
			// udpate the overlay settings of the detached children
			if (child->overlay_)
				child->updateOverlay(false);
			// invalidate the child
			child->invalidate();
			// and detach its root window
			child->detachRootWindow();
            scheduleRelayout();
            repaint();
        }

		void setLayout(Layout* value) {
			ASSERT(value != nullptr) << "use Layout::None instead";
			if (layout_ != value) {
				layout_ = value;
				scheduleRelayout();
				repaint();
			}
		}

		virtual Canvas getChildrenCanvas(Canvas & canvas) {
			// create the child canvas
			return Canvas(canvas).clip(childRect());			
		}

		virtual void setChildGeometry(Widget* child, int x, int y, int width, int height) {
			if (child->x() != x || child->y() != y) {
				if (child->visibleRect_.valid())
					child->invalidateContents();
				child->updatePosition(x, y);
			}
			if (child->width() != width || child->height() != height) {
				if (child->visibleRect_.valid())
				    child->invalidateContents();
				child->updateSize(width, height);
			}
		}

        /** Invalidate the container and its contents and then schedule relayout of the container. 
         */
		void invalidateContents() override {
			Widget::invalidateContents();
            for (Widget * child : children_)
                child->invalidateContents();
			scheduleRelayout();
		}

		/** Action to take when child is invalidated. 

		    This method is called whenever a child is invalidated. The default action is to repaint the widget and relayout its components.
		 */
        void childInvalidated(Widget* child) {
			MARK_AS_UNUSED(child);
			scheduleRelayout();
			repaint();
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

		void paintChild(Widget * child, Canvas & clientCanvas);

		void updateOverlay(bool value) override;

        /** Given mouse coordinates, returns the target widget and updates the coordinates accordingly. 
         */
		virtual Widget * getMouseTarget(int & col, int & row) {
            // mouse coordinates in the client area
            //Point coord{col - border_.left, row - border_.top};
            Point coord{col, row};
            /*Point coord{
                col - clientRect_.left() + scrollOffset_.x,
                row - clientRect_.top() + scrollOffset_.y
            }; */
            for (auto i = children_.rbegin(), e = children_.rend(); i != e; ++i) {
                if ((*i)->rect().contains(coord)) {
                    col = coord.x - (*i)->x();
                    row = coord.y - (*i)->y();
                    return (*i)->getMouseTarget(col, row);
                }
            }
			return this;
		}

		/** Schedules layout of all components on the next repaint event without actually triggering the repaint itself. 
		 */
		void scheduleRelayout() {
			relayout_ = true;
		}

		void attachRootWindow(RootWindow * root) override {
			Widget::attachRootWindow(root);
			for (Widget * child : children_)
				child->attachRootWindow(root);
		}

		void detachRootWindow() override {
			for (Widget * child : children_)
				child->detachRootWindow();
			Widget::detachRootWindow();
		}

	private:

		friend class Layout;

        /** All widgets which have the current widget as their parent.
         */
        std::vector<Widget *> children_;

		Layout* layout_;
		bool relayout_;

	}; // ui::Container

} // namespace ui
#pragma once

#include <vector>

#include "widget.h"

namespace ui {

	class Layout;

	/** Container is a widget capable of managing its children at runtime. 
	 */
	class Container : public virtual Widget {
	public:

		Layout* layout() const {
			return layout_;
		}

	protected:
		friend class Widget;

		Container();

        std::vector<Widget *> const & children() const {
            return children_;
        }

        std::vector<Widget *> & children() {
            return children_;
        }

        virtual void attachChild(Widget * child) {
            // first detach the child if it is attached to other widget
            if (child->parent_ != nullptr)
                child->parent_->detachChild(child);
            // now, under paint lock add the children to the list and patch the parent link
            {
                PaintLockGuard g(this);
                child->parent_ = this;
                children_.push_back(child);
            }
            scheduleRelayout();
            repaint();
        }

        virtual void detachChild(Widget * child) {
            ASSERT(child->parent_ == this);
            // remove the child from list of children first and then detach it under the paint lock
            {
                PaintLockGuard g(this);
                for (auto i = children_.begin(), e = children_.end(); i != e; ++i)
                    if (*i == child) {
                        children_.erase(i);
                        break;
                    }
                child->parent_ = nullptr;
                // udpate the overlay settings of the detached children
                if (child->overlay_)
                    child->updateOverlay(false);
                // invalidate the child
                child->invalidate();
                // and detach its root window
                child->detachRootWindow();
            }
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

		void updateOverlay(bool value) override;

		/** Returns the target for the given mouse coordinates. 
		 
		 */
		Widget* getMouseTarget(int col, int row) override {
			ASSERT(visibleRect_.contains(col, row));
			// go from rbegin so that greatest z-index widget at given coordinates will be selected
			for (auto i = children().rbegin(), e = children().rend(); i != e; ++i) {
				if ((*i)->visible() && (*i)->visibleRect_.contains(col, row))
				    return *i;
			}
			return this;
		}

		/** Schedules layout of all components on the next repaint event without actually triggering the repaint itself. 
		 */
		void scheduleRelayout() {
			relayout_ = true;
		}

		void detachRootWindow() {
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
		using Widget::onMouseOver;
		using Widget::onMouseOut;

		// Methods from Widget

		using Widget::setVisible;
		using Widget::move;
		using Widget::resize;
		using Widget::setWidthHint;
		using Widget::setHeightHint;

		// Methods from Container

		using Container::attachChild;
		using Container::detachChild;
		using Container::setLayout;

		PublicContainer() :
		    background_(Color::Black) {
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
			canvas.fill(Rect::FromWH(canvas.width(), canvas.height()), Brush(Color::Black));
			Container::paint(canvas);
		}

	private:

		Brush background_;




	};

} // namespace ui
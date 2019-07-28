#pragma once

#include "container.h"

namespace ui {

	/** Determines the basic interface of a container layout. 
	
	    The layout is a class responsible for layouting the widgets in the container.
	 */
	class Layout {
	public:

		static Layout* None();
		static Layout* Maximized();
		static Layout* Horizontal();
		static Layout* Vertical();

	protected:
		friend class Container;

		/** Method called on the layout when the container widgets' size and position is to be calculated. 

		    Respective layouts should implement the functionality. 
		 */
		virtual void relayout(Container* container, Canvas const & clientCanvas) = 0;

		/** Calculates overlays for the widgets in the container. 

		    This can be overriden in children to provide better overlaying logic, but the default overlay is quite simple: Iterates the widgets in the container backwards and remembers the rectangle in which the widget fits. If new widgets intersects with the rectangle, its overlay is set to true, otherwise its set to false. 
		 */
		virtual void calculateOverlay(Container* container) {
			Rect overlay;
			for (auto i = childrenOf(container).rbegin(), e = childrenOf(container).rend(); i != e; ++i) {
				if (!(*i)->visible())
					continue;
				Rect childRect = (*i)->rect();
				(*i)->setOverlay(!Rect::Intersection(childRect, overlay).empty());
				overlay = Rect::Union(childRect, overlay);
			}
		}

		/** Returns the children of given container. 
		 */
		std::vector<Widget*> const& childrenOf(Container* container) {
			return container->children_;
		}

		void setChildGeometry(Container* container, Widget* child, int x, int y, int width, int height) {
			container->setChildGeometry(child, x, y, width, height);
		}

		void relayoutWidget(Widget* child, int width, int height) {
			child->relayout(width, height);
		}

	private:
		class NoneImpl;
		class MaximizedImpl;
		class HorizontalImpl;
		class VerticalImpl;
	};

	class Layout::NoneImpl : public Layout {
	protected:
		virtual void relayout(Container* container, Canvas const& clientCanvas) {
			int w = clientCanvas.width();
			int h = clientCanvas.height();
			for (Widget* child : childrenOf(container))
				relayoutWidget(child, w, h);
		}
	};

	class Layout::MaximizedImpl : public Layout {
	protected:
		virtual void relayout(Container* container, Canvas const& clientCanvas) {
			int w = clientCanvas.width();
			int h = clientCanvas.height();
			for (Widget* child : childrenOf(container))
				setChildGeometry(container, child, 0, 0, w, h);
		}

		/** All but the last visible widget in the maximized container are overlaid.
		 */
		virtual void calculateOverlay(Container* container) {
			std::vector<Widget*> const& children = childrenOf(container);
			bool overlay = false;
			for (auto i = children.rbegin(), e = children.rend(); i != e; ++i) {
				if (!(*i)->visible())
					continue;
				(*i)->setOverlay(overlay);
				overlay = false;
			}
		}
	};

	class Layout::HorizontalImpl : public Layout {
	protected:
		virtual void relayout(Container* container, Canvas const& clientCanvas) {
			int w = clientCanvas.width();
			std::vector<Widget*> const& children = childrenOf(container);
			// first determine the number of visible children to layout, the total height occupied by children with fixed height and number of children with height hint set to auto
			size_t visibleChildren = 0;
			size_t autoChildren = 0;
			int fixedHeight = 0;
			for (Widget* child : children)
				if (child->visible()) {
					++visibleChildren;
					if (child->heightHint().isFixed()) {
						fixedHeight += child->height();
						layout(container, child, child->y(), w, child->height());
					} else if (child->heightHint().isAuto()) {
						++autoChildren;
					} 
				}
			// nothing to layout if there are no kids
			if (visibleChildren == 0)
				return;
			// set the width and height of the percentage wanting children
			int totalHeight = clientCanvas.height();
			int availableHeight = totalHeight - fixedHeight;
			for (Widget * child : children)
				if (child->heightHint().isPercentage()) {
					// determine the height 
					int h = (totalHeight - fixedHeight) * child->heightHint().pct() / 100;
					layout(container, child, child->y(), w, h);
					// this is important because the height might have been adjusted by the widget itself when it was set by the layout
					availableHeight -= child->height();
				}
			// now we know the available height for the auto widgets
			if (autoChildren != 0) {
				int h = availableHeight / autoChildren;
				for (Widget* child : children) {
					if (child->heightHint().isAuto()) {
						if (--autoChildren == 0)
							h = availableHeight;
						layout(container, child, child->y(), w, h);
						availableHeight -= child->height();
					}
				}
			}
			// finally, when the sizes are right, we must reposition the elements
			int top = 0;
			for (Widget* child : children) {
				setChildGeometry(container, child, 0, top, child->width(), child->height());
				top += child->height();
			}
		}

		/** None of the children are overlaid.
		 */
		virtual void calculateOverlay(Container* container) {
			for (Widget* child : childrenOf(container))
				child->setOverlay(false);
		}

		void layout(Container * container, Widget* child, int top, int maxWidth, int height) {
			if (child->widthHint().isFixed()) {
				setChildGeometry(container, child, 0, top, child->width(), height);
			} else if (child->widthHint().isAuto()) {
				setChildGeometry(container, child, 0, top, maxWidth, height);
			} else {
				setChildGeometry(container, child, 0, top, maxWidth * child->widthHint().pct() / 100, height);
			}
		}
	};

	class Layout::VerticalImpl : public Layout {
	protected:
		virtual void relayout(Container* container, Canvas const& clientCanvas) {
			int h = clientCanvas.height();
			std::vector<Widget*> const& children = childrenOf(container);
			// first determine the number of visible children to layout, the total width occupied by children with fixed width and number of children with width hint set to auto
			size_t visibleChildren = 0;
			size_t autoChildren = 0;
			int fixedWidth = 0;
			for (Widget* child : children)
				if (child->visible()) {
					++visibleChildren;
					if (child->widthHint().isFixed()) {
						fixedWidth += child->width();
						layout(container, child, child->x(), child->width(), h);
					} else if (child->widthHint().isAuto()) {
						++autoChildren;
					}
				}
			// nothing to layout if there are no kids
			if (visibleChildren == 0)
				return;
			// set the width and height of the percentage wanting children
			int totalWidth = clientCanvas.width();
			int availableWidth = totalWidth - fixedWidth;
			for (Widget* child : children)
				if (child->widthHint().isPercentage()) {
					// determine the height 
					int w = (totalWidth - fixedWidth) * child->widthHint().pct() / 100;
					layout(container, child, child->x(), w, h);
					// this is important because the height might have been adjusted by the widget itself when it was set by the layout
					availableWidth -= child->width();
				}
			// now we know the available height for the auto widgets
			if (autoChildren != 0) {
				int w = availableWidth / autoChildren;
				for (Widget* child : children) {
					if (child->widthHint().isAuto()) {
						if (--autoChildren == 0)
							w = availableWidth;
						layout(container, child, child->x(), w, h);
						availableWidth -= child->width();
					}
				}
			}
			// finally, when the sizes are right, we must reposition the elements
			int left = 0;
			for (Widget* child : children) {
				setChildGeometry(container, child, left, 0, child->width(), child->height());
				left += child->width();
			}
		}

		/** None of the children are overlaid.
		 */
		virtual void calculateOverlay(Container* container) {
			for (Widget* child : childrenOf(container))
				child->setOverlay(false);
		}

		void layout(Container* container, Widget* child, int left, int width, int maxHeight) {
			if (child->heightHint().isFixed()) {
				setChildGeometry(container, child, left, 0, width, child->height());
			} else if (child->heightHint().isAuto()) {
				setChildGeometry(container, child, left, 0, width, maxHeight);
			} else {
				setChildGeometry(container, child, left, 0, width, maxHeight * child->heightHint().pct() / 100);
			}
		}
	};



	inline Layout* Layout::None() {
		static NoneImpl layout;
		return &layout;
	}

	inline Layout* Layout::Maximized() {
		static MaximizedImpl layout;
		return &layout;
	}

	inline Layout* Layout::Horizontal() {
		static HorizontalImpl layout;
		return &layout;
	}

	inline Layout* Layout::Vertical() {
		static VerticalImpl layout;
		return &layout;
	}



} // namespace ui
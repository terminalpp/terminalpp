#pragma once

#include "container.h"

namespace ui {

	/** Determines the basic interface of a container layout. 
	
	    The Layout is responsible for determining the position and size of container's children when the container, or its children are resized. A layout implementation is responsible for two things, namely repositioning the children via the relayout() method and determining their overlay via the calculateOverlay() method, both of which can be updated in the subclasses.

		Layouts are expected to be global objects and are never deleted by the containers. 

		Common layouts can be obtained via static methods in the Layout class.
	 */
	class Layout {
	public:

		/** Returns default layout implementation, which just delegates the layouting to the children widget's themselves. 
		 */
		static Layout* None();

		/** Returns a layout which maximizes all children of the container. 
		 */
		static Layout* Maximized();

		/** Returns a layoyt which organizes the children horizontally. 
		 */
		static Layout* Horizontal();

		/** Returns a layout which organizes the children vertically. 
		 */
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
	}; // ui::Layout

	/** No layout, where any of the layouting decissions are forwarded to the children widgets via their relayout() method. 
	 */
	class Layout::NoneImpl : public Layout {
	protected:
		virtual void relayout(Container* container, Canvas const& clientCanvas) {
			int w = clientCanvas.width();
			int h = clientCanvas.height();
			for (Widget* child : childrenOf(container))
				relayoutWidget(child, w, h);
		}
	}; // Layout::NoneImpl

	/** Layout which maximizes the children widgets to fill the entire area of the parent container. 

	    If the children cannot be maximized (i.e. their size hints prevent that, or their resize update sets different values, the widget is centered with its size. 
	 */
	class Layout::MaximizedImpl : public Layout {
	protected:
		virtual void relayout(Container* container, Canvas const& clientCanvas) {
			int w = clientCanvas.width();
			int h = clientCanvas.height();
			for (Widget* child : childrenOf(container))
				layout(container, child, w, h);
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

		/** Layouts the child according to its size hints and positions it in the center of the parent's canvas.
		 */
		void layout(Container* container, Widget* child, int maxWidth, int maxHeight) {
			int w = maxWidth;
			if (child->widthHint().isFixed())
				w = child->width();
			else if (child->widthHint().isPercentage())
				w = maxWidth * child->widthHint().pct() / 100;
			int h = maxHeight;
			if (child->heightHint().isFixed())
				h = child->width();
			else if (child->heightHint().isPercentage())
				h = maxHeight * child->heightHint().pct() / 100;
			int l = (w == maxWidth) ? 0 : ((maxWidth - w) / 2);
			int t = (h == maxHeight) ? 0 : ((maxHeight - h) / 2);
			setChildGeometry(container, child, l, t, w, h);
			// if the child altered its size, we must reposition it
			if (child->width() != w || child->height() != h) {
				w = child->width();
				h = child->height();
				l = ((maxWidth - w) / 2);
				t = ((maxHeight - h) / 2);
				setChildGeometry(container, child, l, t, w, h);
			}
		}
	}; // Layout::MaximizedImpl

	/** Layouts all the children as horizontally stacked bars. 

	    Does not resize widgets with fixed-size hint, all percentage hints are relative to the total available height - the fixed widget's height being 100%. The autosized widgets will each get equal share of the remaining available height. The last autosized widget fills up the rest of the parent's container. 

		Unless hinted otherwise, width of all children will be set to the width of the parent's canvas.
	 */
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
			if (child->widthHint().isFixed()) 
				setChildGeometry(container, child, 0, top, child->width(), height);
			else if (child->widthHint().isAuto()) 
				setChildGeometry(container, child, 0, top, maxWidth, height);
			else 
				setChildGeometry(container, child, 0, top, maxWidth * child->widthHint().pct() / 100, height);
		}
	}; // Layout::HorizontalImpl

	/** Layouts all the children as vertically stacked bars.

		Does not resize widgets with fixed-size hint, all percentage hints are relative to the total available width - the fixed widget's width being 100%. The autosized widgets will each get equal share of the remaining available width. The last autosized widget fills up the rest of the parent's container.

		Unless hinted otherwise, height of all children will be set to the height of the parent's canvas.
	 */
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
			if (child->heightHint().isFixed())
				setChildGeometry(container, child, left, 0, width, child->height());
			else if (child->heightHint().isAuto())
				setChildGeometry(container, child, left, 0, width, maxHeight);
			else
				setChildGeometry(container, child, left, 0, width, maxHeight * child->heightHint().pct() / 100);
		}
	}; // Layout::VerticalImpl

	// actual implementation of layout getters now that we have implementations of the actual layouts

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
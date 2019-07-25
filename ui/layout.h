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

	protected:
		friend class Container;

		/** Method called on the layout when the container widgets' size and position is to be calculated. 

		    Respective layouts should implement the functionality. 
		 */
		virtual void relayout(Container* container) = 0;


		/** Calculates overlays for the widgets in the container. 

		    This can be overriden in children to provide better overlaying logic, but the default overlay is quite simple: Iterates the widgets in the container backwards and remembers the rectangle in which the widget fits. If new widgets intersects with the rectangle, its overlay is set to true, otherwise its set to false. 
		 */
		virtual void calculateOverlay(Container* container) {
			Rect overlay;
			for (auto i = childrenOf(container).rbegin(), e = childrenOf(container).rend(); i != e; ++i) {
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

		void setChildGeometry(Container* container, Widget* child, int x, int y, unsigned width, unsigned height) {
			container->setChildGeometry(child, x, y, width, height);
		}


	private:
		class NoneImpl;
		class MaximizedImpl;
		class HorizontalImpl;
	};

	class Layout::NoneImpl : public Layout {
	protected:
		virtual void relayout(Container* container) {
			MARK_AS_UNUSED(container);
		}
	};

	class Layout::MaximizedImpl : public Layout {
	protected:
		virtual void relayout(Container* container) {
			for (Widget* child : childrenOf(container))
				setChildGeometry(container, child, 0, 0, container->width(), container->height());
		}

		/** All but the last widget in the maximized container are overlaid.
		 */
		virtual void calculateOverlay(Container* container) {
			std::vector<Widget*> const& children = childrenOf(container);
			auto i = children.rbegin(), e = children.rend();
			if (i != e) {
				(*i++)->setOverlay(false);
				while (i != e)
					(*i++)->setOverlay(true);
			}
		}
	};

	class Layout::HorizontalImpl : public Layout {
	protected:
		virtual void relayout(Container* container) {
			std::vector<Widget*> const& children = childrenOf(container);
			// nothing to layout if there are no kids
			if (children.empty())
				return;
			unsigned totalHeight = container->height();
			unsigned h = totalHeight / children.size();
			int top = 0;
			for (Widget* child : children) {
				setChildGeometry(container, child, 0, top, container->width(), h);
				top += h;
				if (top + (h * 2) > totalHeight)
					h = totalHeight - top;
			}
		}

		/** None of the children are overlaid.
		 */
		virtual void calculateOverlay(Container* container) {
			for (Widget* child : childrenOf(container))
				child->setOverlay(false);
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



} // namespace ui
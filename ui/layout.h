#pragma once

#include "container.h"

namespace ui {

	/** Determines the basic interface of a container layout. 
	
	    The layout is a class responsible for layouting the widgets in the container.
	 */
	class Layout {
	public:

		static Layout* None();

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

	private:
		class NoneImpl;

	};

	class Layout::NoneImpl : public Layout {
	protected:
		virtual void relayout(Container* container) {
			MARK_AS_UNUSED(container);
		}
	};


	inline Layout* Layout::None() {
		static NoneImpl layout;
		return &layout;
	}



} // namespace ui
#pragma once

#include "canvas.h"

namespace ui {

	class Widget;
	class Container;

	/** Determines the basic interface of a container layout. 
	
	    The Layout is responsible for determining the position and size of container's children when the container, or its children are resized. A layout implementation is responsible for two things, namely repositioning the children via the relayout() method and determining their overlay via the calculateOverlay() method, both of which can be updated in the subclasses.

		Layouts are expected to be global objects and are never deleted by the containers. 

		Common layouts can be obtained via static methods in the Layout class.
	 */
	class Layout {
	public:

	    class SizeHint;

		/** Returns default layout implementation, which just delegates the layouting to the children widget's themselves. 
		 */
		static Layout * None;

		/** Returns a layout which maximizes all children of the container. 
		 */
		static Layout * Maximized;

		/** Returns a layoyt which organizes the children horizontally. 
		 */
		static Layout * Horizontal;

		/** Returns a layout which organizes the children vertically. 
		 */
		static Layout * Vertical;

	protected:
		friend class Container;

		/** Method called on the layout when the container widgets' size and position is to be calculated. 

		    Respective layouts should implement the functionality. 
		 */
		virtual void relayout(Container* container, Canvas const & clientCanvas) = 0;

		/** Calculates overlays for the widgets in the container. 

		    This can be overriden in children to provide better overlaying logic, but the default overlay is quite simple: Iterates the widgets in the container backwards and remembers the rectangle in which the widget fits. If new widgets intersects with the rectangle, its overlay is set to true, otherwise its set to false. 
		 */
		virtual void calculateOverlay(Container* container);

		/** Returns the children of given container. 
		 */
		std::vector<Widget*> const& childrenOf(Container* container);

		void setChildGeometry(Container* container, Widget* child, int x, int y, int width, int height) ;

		void setOverlayOf(Widget * child, bool value);

		void relayoutWidget(Widget* child, int width, int height);

	}; // ui::Layout

	/** Size hint provides hints about the width and height of a widget to the layouting engine. 

	    Can be:

		- auto (left to the layout engine, if any)
		- fixed (not allowed to touch the present value)
		- percentage (percentage of the parent)
	 */
	class Layout::SizeHint {
	public:

		static SizeHint Auto() {
			return SizeHint(AUTO);
		}

		static SizeHint Fixed() {
			return SizeHint(FIXED);
		}

		static SizeHint Percentage(unsigned value) {
			ASSERT(value <= 100);
			return SizeHint(PERCENTAGE + value);
		}

		int pct() const {
			ASSERT(raw_ && PERCENTAGE) << "Not a percentage size hint";
			return static_cast<int>(raw_ & 0xff);
		}

		bool isFixed() const {
			return raw_ & FIXED;
		}

		bool isAuto() const {
			return raw_ & AUTO;
		}

		bool isPercentage() const {
			return raw_ & PERCENTAGE;
		}

		bool operator == (SizeHint other) const {
			return raw_ == other.raw_;
		}

		bool operator != (SizeHint other) const {
			return raw_ != other.raw_;
		}

	private:
		static constexpr unsigned AUTO = 0x100;
		static constexpr unsigned FIXED = 0x200;
		static constexpr unsigned PERCENTAGE = 0x400;

		SizeHint(unsigned raw) :
			raw_(raw) {
		}

		unsigned raw_;
	}; // ui::Layout::SizeHint



} // namespace ui
#ifdef FOO
#include "layout.h"

#include "control.h"
#include "container.h"

namespace ui {

	std::vector<Control*>& Layout::childrenOf(Container* container) {
		return container->children_;
	}

	// maximized layout

	void Layout::MaximizedImplementation::resized(Container* container) {
		for (Control * child : childrenOf(container))
			forceGeometry(child, 0, 0, container->width(), container->height());
	}

	void Layout::MaximizedImplementation::childChanged(Container* container, Control* control) {
		forceGeometry(control, 0, 0, container->width(), container->height());
	}

	// vertical layout

	void Layout::VerticalImplementation::resized(Container* container) {
		NOT_IMPLEMENTED;
	}

	void Layout::VerticalImplementation::childChanged(Container* container, Control* control) {
		NOT_IMPLEMENTED;
	}

	// horizontal layout

	void Layout::HorizontalImplementation::resized(Container* container) {
		std::vector<Control*> & children = childrenOf(container);
		unsigned totalHeight = container->height();
		unsigned totalPct = 0;
		unsigned fixedHeight = 0;
		unsigned autoControls = 0;
		for (Control* child : children) {
			switch (child->heightHint()) {
				case SizeHint::Kind::Fixed:
					fixedHeight += child->height();
					break;
				case SizeHint::Kind::Auto:
					++autoControls;
					break;
				case SizeHint::Kind::Percentage:
					totalPct += child->heightHint().value();
					break;
				default:
					UNREACHABLE;
			}
		}
		// this is the height available for percentage based controls
		totalHeight -= fixedHeight;
		unsigned percentageHeight = totalHeight;
		if (totalPct > 0) {
			for (Control* child : children)
				if (child->heightHint() == SizeHint::Kind::Percentage)
					totalHeight -= percentageHeight * child->heightHint().value() / 100;
		}
		// height of auto sized control
		unsigned h = totalHeight / ((autoControls == 0) ? 1 : autoControls);
		unsigned top = 0;
		// we are now ready to update the geometries
		for (Control* child : children) {
			unsigned width = child->widthHint().calculate(
				child->width(), 
				container->width(), 
				container->width()
			);
			unsigned height = child->heightHint().calculate(
				child->height(),
				h,
				percentageHeight
			);
			forceGeometry(child, 0, top, width, height);
			top += height;
			if (child->heightHint() == SizeHint::Kind::Auto) {
				totalHeight -= h;
				if (--autoControls == 1)
					h = totalHeight;
			}
		}
	}

	void Layout::HorizontalImplementation::childChanged(Container* container, Control* control) {
		resized(container);
	}

} // namespace ui
#endif
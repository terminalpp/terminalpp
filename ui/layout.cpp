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
		unsigned h = container->height() / children.size();
		unsigned top = 0;
		for (size_t i = 0, e = children.size() - 1; i <= e; ++i) {
			unsigned ch = (i < e) ? h : container->height() - top;
			forceGeometry(children[i], 0, top, container->width(), ch);
			top += ch;
		}
	}

	void Layout::HorizontalImplementation::childChanged(Container* container, Control* control) {
		resized(container);
	}

} // namespace ui
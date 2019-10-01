#include "layout.h"

#include "container.h"

namespace ui {

	Container::Container() :
		layout_(Layout::None()),
		relayout_(true) {
	}

	Widget * Container::mouseDown(int col, int row, ui::MouseButton button, ui::Key modifiers) {
		Widget * target = getMouseTarget(col, row);
		if (target != nullptr)
		    return target->mouseDown(col, row, button, modifiers);
		// if there is no target in children, the container itself is the target
		return Widget::mouseDown(col, row, button, modifiers);
	}

	Widget * Container::mouseUp(int col, int row, ui::MouseButton button, ui::Key modifiers) {
		Widget * target = getMouseTarget(col, row);
		if (target != nullptr)
		    return target->mouseDown(col, row, button, modifiers);
		// if there is no target in children, the container itself is the target
		return Widget::mouseUp(col, row, button, modifiers);
	}

	void Container::mouseClick(int col, int row, ui::MouseButton button, ui::Key modifiers) {
		Widget * target = getMouseTarget(col, row);
		if (target != nullptr)
		    return target->mouseClick(col, row, button, modifiers);
		// if there is no target in children, the container itself is the target
		return Widget::mouseClick(col, row, button, modifiers);
	}

	void Container::mouseDoubleClick(int col, int row, ui::MouseButton button, ui::Key modifiers) {
		Widget * target = getMouseTarget(col, row);
		if (target != nullptr)
		    return target->mouseDoubleClick(col, row, button, modifiers);
		// if there is no target in children, the container itself is the target
		return Widget::mouseDoubleClick(col, row, button, modifiers);
	}

	void Container::mouseWheel(int col, int row, int by, Key modifiers) {
		Widget * target = getMouseTarget(col, row);
		if (target != nullptr)
		    return target->mouseWheel(col, row, by, modifiers);
		// if there is no target in children, the container itself is the target
		return Widget::mouseWheel(col, row, by, modifiers);
	}

	Widget * Container::mouseMove(int col, int row, Key modifiers) {
		Widget * target = getMouseTarget(col, row);
		if (target != nullptr)
		    return target->mouseMove(col, row, modifiers);
		// if there is no target in children, the container itself is the target
		return Widget::mouseMove(col, row, modifiers);
	}

	void Container::paint(Canvas& canvas) {
		Canvas clientCanvas{getClientCanvas(canvas)};
		if (relayout_) {
			layout_->relayout(this, clientCanvas);
			if (!overlay_)
				layout_->calculateOverlay(this);
			relayout_ = false;
		}
		// display the children
		for (Widget* child : children())
			paintChild(child, clientCanvas);
	}

	void Container::updateOverlay(bool value) {
		Widget::updateOverlay(value);
		// if the container itself is overlaid, then all its children are overlaid too as they may be part of the parent's overlay
		if (value) {
			for (Widget* child : children())
				child->updateOverlay(true);
		} 
		// make sure that next repaint will relayout all children, which also sets their overlay flags
		relayout_ = true;

	}

} // namespace ui
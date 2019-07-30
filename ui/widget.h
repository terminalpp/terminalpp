#pragma once

#include "helpers/helpers.h"

#include "canvas.h"

namespace ui {

	/** Size hint provides hints about the width and height of a widget to the layouting engine. 

	    Can be:

		- auto (left to the layout engine, if any)
		- fixed (not allowed to touch the present value)
		- percentage (percentage of the parent)
	 */
	class SizeHint {
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
	};

	template<typename T>
	using Event = helpers::Event<T>;

	typedef vterm::MouseButton MouseButton;
	typedef vterm::Key Key;

	class Widget;

	class MouseButtonPayload {
	public:
		int x;
		int y;
		MouseButton button;
		Key modifiers;
	};

	class MouseWheelPayload {
	public:
		int x;
		int y;
		int by;
		Key modifiers;
	};

	class MouseMovePayload {
	public:
		int x;
		int y;
		Key modifiers;
	};

	typedef helpers::EventPayload<void, ui::Widget> NoPayloadEvent;

	typedef helpers::EventPayload<MouseButtonPayload, ui::Widget> MouseButtonEvent;
	typedef helpers::EventPayload<MouseWheelPayload, ui::Widget> MouseWheelEvent;
	typedef helpers::EventPayload<MouseMovePayload, ui::Widget> MouseMoveEvent;

	/** Base class for all UI widgets. 

	    The widget manages the basic properties of every ui element, namely the position, size, visibility, drawing of its contents and events corresponding to this functionality as well as basic input & output events from the terminal (mouse, keyboard and clipboard). 
	 */
	class Widget : virtual public helpers::Object {
	protected:

		// events

		/** Triggered when visibility changes to true. 
		 */
		Event<NoPayloadEvent> onShow;

		/** Triggered when visibility changes to false. 
		 */
		Event<NoPayloadEvent> onHide;

		/** Triggered when the widget's size has been updated. 
		 */
		Event<NoPayloadEvent> onResize;

		/** Triggered when the widget's position has been updated. 
		 */
		Event<NoPayloadEvent> onMove;

		/** Triggered when the widget has obtained focus, i.e. it will receive keyboard events. 
		 */
		Event<NoPayloadEvent> onFocusIn;

		/** Triggered when the widget has lost focus, i.e. it will no longer receive keyboard events. 
		 */
		Event<NoPayloadEvent> onFocusOut;

		Event<MouseButtonEvent> onMouseDown;
		Event<MouseButtonEvent> onMouseUp;
		Event<MouseButtonEvent> onMouseClick;
		Event<MouseButtonEvent> onMouseDoubleClick;
		Event<MouseWheelEvent> onMouseWheel;
		Event<MouseMoveEvent> onMouseMove;
		Event<NoPayloadEvent> onMouseEnter;
		Event<NoPayloadEvent> onMouseLeave;


	public:

		Widget(int x, int y, int width, int height):
			parent_(nullptr),
			visibleRegion_(nullptr),
			overlay_(false),
			forceOverlay_(false),
			visible_(true),
			x_(x),
			y_(y),
			width_(width),
			height_(height),
			widthHint_(SizeHint::Auto()),
			heightHint_(SizeHint::Auto()) {
		}

		Widget* parent() const {
			return parent_;
		}

		bool visible() const {
			return visible_;
		}

		/** Returns the x and y coordinates of the top-left corner of the widget in its parent 
		 */
		int x() const {
			return x_;
		}

		int y() const {
			return y_;
		}

		int width() const {
			return width_;
		}

		int height() const {
			return height_;
		}

		SizeHint widthHint() const {
			return widthHint_;
		}

		SizeHint heightHint() const {
			return heightHint_;
		}

		Rect rect() const {
			return Rect(x_, y_, x_ + width_, y_ + height_);
		}

		/** Repaints the widget.

			Only repaints the widget if the visibleRegion is valid. If the visible region is invalid, does nothing because when the region was invalidated, the repaint was automatically triggered, so there is either repaint pending, or in progress.
		 */
		void repaint();

    protected:

		/** Sets the widget as visible or hidden.

		    Also triggers the repaint of entire parent, because the widget may interfere with other children of its own parent.
		 */
		void setVisible(bool value) {
			if (visible_ != value) {
				if (value) {
					visible_ = true;
					if (parent_ != nullptr)
					    parent_->childInvalidated(this);
					trigger(onShow);
				} else {
					visible_ = false;
					if (parent_ != nullptr)
						parent_->childInvalidated(this);
					trigger(onHide);
				}
			}
		}

		/** Moves the widget to the given coordinates relative to its parent.
		 */
		void move(int x, int y) {
			if (x_ != x || y_ != y) {
				updatePosition(x, y);
				repaint();
			}
		}

		/** Resizes the widget. 
		 */
		void resize(int width, int height) {
			if (width_ != width || height_ != height) {
				updateSize(width, height);
				repaint();
			}
		}

		void setOverlay(bool value) {
			if (value != overlay_)
				updateOverlay(value);
		}


		void setWidthHint(SizeHint value) {
			if (widthHint_ != value) {
				widthHint_ = value;
				if (parent_ != nullptr)
					parent_->childInvalidated(this);
			}
		}

		void setHeightHint(SizeHint value) {
			if (heightHint_ != value) {
				heightHint_ = value;
				if (parent_ != nullptr)
					parent_->childInvalidated(this);
			}
		}

	protected:

		bool forceOverlay() const {
			return forceOverlay_;
		}

		void setForceOverlay(bool value) {
			if (forceOverlay_ != value) {
				forceOverlay_ = value;
			}
		}

		virtual void mouseDown(int col, int row, MouseButton button, Key modifiers) {
			MouseButtonPayload e{ col, row, button, modifiers };
			trigger(onMouseUp, e);
		}

		virtual void mouseUp(int col, int row, MouseButton button, Key modifiers) {
			MouseButtonPayload e{ col, row, button, modifiers };
			trigger(onMouseUp, e);
		}

		virtual void mouseClick(int col, int row, MouseButton button, Key modifiers) {
			MouseButtonPayload e{ col, row, button, modifiers };
			trigger(onMouseClick, e);
		}

		virtual void mouseDoubleClick(int col, int row, MouseButton button, Key modifiers) {
			MouseButtonPayload e{ col, row, button, modifiers };
			trigger(onMouseDoubleClick, e);
		}

		virtual void mouseWheel(int col, int row, int by, Key modifiers) {
			MouseWheelPayload e{ col, row, by, modifiers };
			trigger(onMouseWheel, e);
		}

		virtual void mouseMove(int col, int row, Key modifiers) {
			MouseMovePayload e{ col, row, modifiers };
			trigger(onMouseMove, e);
		}

		virtual void mouseEnter() {
			trigger(onMouseEnter);
		}

		virtual void mouseLeave() {
			trigger(onMouseLeave);
		}

		/** Paints given child.

		    Expects the clientCanvas of the parent as the second argument. In cases where border is 0, this can be the widget's main canvas as well. In other cases the getClientCanvas method should be used to obtain the client canvas first. 
		 */
		void paintChild(Widget * child, Canvas& clientCanvas);

		/** Given a canvas for the full widget, returns a canvas for the client area only. 
		 */
		virtual Canvas getClientCanvas(Canvas& canvas) {
			return Canvas(canvas, 0, 0, width(), height());
		}

		/** Invalidates the widget and request its parent repaint,

		    If the widget is valid, invalidates its visible region and informs its parent that a child was invalidated. If the widget is already not valid, does nothing because the parent has already been notified. 
		 */
		void invalidate() {
			if (visibleRegion_.isValid()) {
				invalidateContents();
				if (parent_ != nullptr)
					parent_->childInvalidated(this);
			}
		}

		/** Invalidates the contents of the widget. 
		
		    For simple widgets this only means invalidating the widget's visible region, for more complex widgets all their children must be invalidated too. 
		 */
		virtual void invalidateContents() {
			visibleRegion_.invalidate();
		}

		/** Action to take when child is invalidated. 

		    This method is called whenever a child is invalidated. The default action is to repaint the widget.
		 */
		virtual void childInvalidated(Widget* child) {
			MARK_AS_UNUSED(child);
			repaint();
		}

		/** Updates the position of the widget. 
		
		    Already assumes that the new position is different from the current position. However, in case the requested position is invalid, the widget may adjust the size before setting it. After the size is updated, the control is invalidated and onMove event triggered. 
		 */
		virtual void updatePosition(int x, int y) {
			x_ = x;
			y_ = y;
			invalidate();
			trigger(onMove);
		}

		/** Updates the size of the widget. 
		
		    Assumes the size is different that current size. However, if the size is invalid, the widget may choose to update the requested width and height accordingly. Invalidates the widget and triggers the onResize event. 
		 */
		virtual void updateSize(int width, int height) {
			ASSERT(width >= 0 && height >= 0);
			width_ = width;
			height_ = height;
			invalidate();
			trigger(onResize);
		}

		/** Re-layouts the control within its parent. 

		    This method can be called by the parent's layout in cases the parent allows the children to layout themselves. It should update the position of the widget according to the provided parent width and height. 

			If the parent's layout has own layout specification, then this method will not be called. 
		 */
		virtual void relayout(int parentWidth, int parentHeight) {
			MARK_AS_UNUSED(parentWidth);
			MARK_AS_UNUSED(parentHeight);
			// TODO do fancy stuff like anchors, etc. 
		}

		/** Paints the widget's contents on the provided canvas. 
		 */
		virtual void paint(Canvas& canvas) = 0;

		virtual void updateParent(Widget* parent) {
			if (parent == nullptr) {
				parent_ = nullptr;
				if (overlay_ != false)
				    updateOverlay(false);
			} else {
				ASSERT(parent_ == nullptr);
				parent_ = parent;
				// parent's repaint will eventually trigger the overlay update
			}
		}

		virtual void updateOverlay(bool value) {
			overlay_ = value;
		}

		virtual Widget* getMouseTarget(unsigned col, unsigned row) {
			ASSERT(visibleRegion_.contains(col, row));
			return this;
		}

		/** Updated trigger function for events which takes the Widget as base class for event sender.
		 */
		template<typename EVENT>
		void trigger(EVENT& e) {
			static_assert(std::is_same<typename EVENT::Payload::Sender, Widget >::value, "Only events with sender being ui::Widget should be used");
			typename EVENT::Payload p(this);
			e.trigger(p);
		}

		/** Updated trigger function for events which takes the Widget as base class for event sender.
		 */
		template<typename EVENT>
		void trigger(EVENT& e, typename EVENT::Payload::Payload const& payload) {
			static_assert(std::is_same<typename EVENT::Payload::Sender, Widget >::value, "Only events with sender being ui::Widget should be used");
			typename EVENT::Payload p(this, payload);
			e.trigger(p);
		}

	private:

		friend class Layout;
		friend class Container;
		friend class RootWindow;

		/* Parent widget or none. 
		 */
		Widget* parent_;

		/* Visible region of the canvas. 
		 */
		Canvas::VisibleRegion visibleRegion_;

		/* If true, the rectangle of the widget is shared with other widgets, i.e. when the widget is to be repainted, its parent must be repainted instead. */
		bool overlay_;

		/** Forces the overlay to be always true. This is especially useful for controls with transparent backgrounds. */
		bool forceOverlay_;

		/* Visibility */
		bool visible_;

		/* Position */
		int x_;
		int y_;

		/* Size */
		int width_;
		int height_;

		SizeHint widthHint_;
		SizeHint heightHint_;

	};

	/** Public widget simply exposes the widget's protected events and methods as public ones. 

	    It is a suitable class for most user available controls. 
	 */
	class PublicWidget : public Widget {
	public:

		// Events 

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
		using Widget::onMouseEnter;
		using Widget::onMouseLeave;

		// Methods 

		using Widget::setVisible;
		using Widget::move;
		using Widget::resize;
		using Widget::setWidthHint;
		using Widget::setHeightHint;

		PublicWidget(int x = 0, int y = 0, int width = 1, int height = 1) :
			Widget(x, y, width, height) {
		}

	}; // ui::Control

} // namespace ui



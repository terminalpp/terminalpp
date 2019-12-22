#pragma once

#include "helpers/helpers.h"
#include "helpers/events.h"
#include "helpers/log.h"

#include "canvas.h"
#include "mouse.h"
#include "key.h"
#include "layout.h"

/** \page uiwidget ui - Widget
  
    \brief Basic information about the ui::Widget class and its usage. 

	The ui::Widget is the base class of all user elements visible in the interface. 

	\section widgetPosSize Position, Size and Rectangles

	The position and size of widget and its children/parents is determined by the following rectangles and point. Each widget defines these via the functions described below and with the exception of ui::Widget::Rect(), these can be overriden in child classes to specify extra behavior (see the notes below for examples):

	ui::Widget::rect() returns the widget's rectangle in its parent's client coordinates (see below). The rectangle is composed of the `x` and `y` offsets of the widget and its `width` and `height`. 

	ui::Widget::childRect() returns the rectangle of the widget's area that is available for the widget's children to draw on. Its implementation defaults to a rectangle `[0,0,width,height]`, which means that the entire area of the widget is available for its children. 

	> Widgets with features such as borders should overwrite this method to restrict the children available area only to subset of their own. For instance if a widget with `rect` of `[100,100,10,10]` wants to have 2 rows border at the top and 1 cell border on all other sides, its `childRect` would be `[1,2,9,9]`. 

	ui::Widget::clientRect() returns the area the widget presents to its children. The client rectangle should always start at `[0,0]`, while its width and height can be smaller, equal to, or greater than the `childRect`'s dimensions. If the `clientRect` is greater than `childRect`, scrolling may be used to determine which part of the `clientRect` will map to the visible area of the widget. 

	ui::Widget::scrollOffset() returns the scrolling offset. The scrolling offset determines the point in the `clientRect` that corresponds to the op-left corner of the `childRect`. 

	> A scrollable widget with `rect` of `[100,100,10,10]` has width and height of `10` cells and its top-left corner will be displayed at the `[100,100]` coordinates of its parent's clientRect. The `childRect` will again be `[1,2,9,9]`, corresponding to border of 2 rows at the top and 1 row/cell at other sides. The `clientRect` is `[0,0,100,100]`, which means that although the widget is capable of drawing only `8` cols by `7` rows, it presents an area of `[100,100]` cells and rows to its client. If the `scrollOffset` is `[0,0]` (the default), then only first 8 cols and 7 rows will be visible. If however the `scrollOffset` is `[10,20]`, then the visible rectangle of `clientRect` will be `[10,20,18,27]`.
	
    \section widgetDrawing Drawing the Widgets

	Drawing of the widget is done in their ui::Widget::paint() method, which takes a Canvas as an argument. The Widget class and other base UI classes make sure that repainting occurs when it should and only the necessary part of the screen is updated at each paint event, details of which are given later in this section. 



	\subsection widgetCanvas Canvas

	The ui::Canvas class provides the basic drawing tools, such as text, borders, fills, etc. Furthermore the Canvas also keeps track of its visible region, i.e. the subset of the canvas that is visible, and therefore is backed by proper screen buffer cells. Only updates to the visible region of the canvas will actually have observable effect, but canvas users do not have to worry about which region is visible. 

	> For performance reasons, large widgets may choose to limit their drawing to the visible area only, but this is not required. 

	Each widget remembers its visible canvas region and when painting should occur, uses it to create actual canvas object, which is then passed to the ui::Widget::paint() method.

	> For more details on drawing tools available, see the documentation of the `ui::Canvas` class itself. 
	
    \subsection widgetInvalidation Invalidating a widget




	ui::Widget::getClientCanvas()

	When the widget is drawn, the children of the widget should be drawn next, if any. The children may not necessarily be drawn using the same canvas as the widget and the task of this method is to return the canvas on which the children should be drawn given the widget's own canvas as an argument. 

	The default implementation is to return the widget's own canvas, but the method is overriden in subclasses for purposes such as scrollable contents (in which case the client canvas has to be moved according to the scrolling offset), or when the widget contains borders (so that the client canvas does not allow drawing over them). 

 */

namespace ui {

	class Widget;
	class Container;

	template<typename P, typename T = Widget>
	using Event = helpers::Event<P, T>;

	class MouseButtonEvent {
	public:
		int x;	
		int y;
		MouseButton button;
		Key modifiers;
	};

	class MouseWheelEvent {
	public:
		int x;
		int y;
		int by;
		Key modifiers;
	};

	class MouseMoveEvent {
	public:
		int x;
		int y;
		Key modifiers;
	};

	/** Base class for all UI widgets. 

	    The widget manages the basic properties of every ui element, namely the position, size, visibility, drawing of its contents and events corresponding to this functionality as well as basic input & output events from the terminal (mouse, keyboard and clipboard). 
	 */
	class Widget {
	protected:

		// events

		/** Triggered when visibility changes to true. 
		 */
		Event<void> onShow;

		/** Triggered when visibility changes to false. 
		 */
		Event<void> onHide;

		/** Triggered when the widget's size has been updated. 
		 */
		Event<void> onResize;

		/** Triggered when the widget's position has been updated. 
		 */
		Event<void> onMove;

		/** Triggered when the widget has obtained focus, i.e. it will receive keyboard events. 
		 */
		Event<void> onFocusIn;

		/** Triggered when the widget has lost focus, i.e. it will no longer receive keyboard events. 
		 */
		Event<void> onFocusOut;

		/** Triggered when the widget has been enabled or disabled. 
		 */
		Event<void> onEnabled;
		Event<void> onDisabled;

		Event<MouseButtonEvent> onMouseDown;
		Event<MouseButtonEvent> onMouseUp;
		Event<MouseButtonEvent> onMouseClick;
		Event<MouseButtonEvent> onMouseDoubleClick;
		Event<MouseWheelEvent> onMouseWheel;
		Event<MouseMoveEvent> onMouseMove;
		Event<void> onMouseOver;
		Event<void> onMouseOut;


		// keyboard events

	public:

        virtual ~Widget();

		Widget():
			parent_(nullptr),
			visibleRect_{},
			overlay_(false),
			forceOverlay_(false),
			visible_(true),
			enabled_(true),
			focused_(false),
			focusStop_(false),
			focusIndex_(0),
			x_(0),
			y_(0),
			width_(0),
			height_(0),
			widthHint_(Layout::SizeHint::Auto()),
			heightHint_(Layout::SizeHint::Auto()) {
		}

		Widget(Widget &&) = delete;

		Container* parent() const {
			return parent_;
		}

		/** Returns the root window of the widget, or nullptr if the window is not attached.
		 */
		RootWindow * rootWindow() const {
			return visibleRect_.rootWindow();
		}

		bool visible() const {
			return visible_;
		}

		bool enabled() const {
			return enabled_;
		}

		/** Returns true if the widget has keyboard focus. 
		 */
		bool focused() const {
			return focused_;
		}

		/** Returns true if the widget is a focus stop, i.e. if the widget can be explicitly selected by the user. 
		 */
		bool focusStop() const {
			return focusStop_;
		}

		/** Returns the focus index, 
		 
		    Focus indexia a number that determines the order of the explicit focus acquisition by the user for focus stoppable elements. 
		 */
		unsigned focusIndex() const {
			return focusIndex_;
		}

		/** Returns the x coordinate of the top-left corner of the widget in its parent 
		 */
		int x() const {
			return x_;
		}

		/** Returns the y coordinate of the top-left corner of the widget in its parent 
		 */
		int y() const {
			return y_;
		}

		int width() const {
			return width_;
		}

		int height() const {
			return height_;
		}

		/** Returns the rectangle of the widget in its parent's client rectangle. 
		 */
		Rect rect() const {
			return Rect::FromCorners(x_, y_, x_ + width_, y_ + height_);
		}

		/** Returns the subset of the widget's canvas rectangle that is available to widget's children. 
		 
		    The default implementation returns the entire rectangle of the widget. 
		 */
		virtual Rect childRect() const {
			return Rect::FromWH(width_, height_);
		}

		Layout::SizeHint widthHint() const {
			return widthHint_;
		}

		Layout::SizeHint heightHint() const {
			return heightHint_;
		}

		/** Repaints the widget.

			Only repaints the widget if the VisibleRect is valid. If the visible region is invalid, does nothing because when the region was invalidated, the repaint was automatically triggered, so there is either repaint pending, or in progress.
		 */
		virtual void repaint();

    protected:

		/** Sets the widget as visible or hidden.

		    Also triggers the repaint of entire parent, because the widget may interfere with other children of its own parent.
		 */
		void setVisible(bool value);

		void setEnabled(bool value);

		/** Focuses or defocuses the widget. 
		 
		    Relays the request to the root window, which means that only valid widgets can be focused or defocused.
		 */
		void setFocused(bool value);

		/** Sets whether the widget acts as keboard focus stop, i.e. if it can be explicitly focused by the user. 
		 */
		void setFocusStop(bool value) {
			if (focusStop_ != value)
			    updateFocusStop(value);
		}

		/** Sets the order of explicit keyboard focus activation. 
		 */
		void setFocusIndex(unsigned value) {
			if (focusIndex_ != value)
			    updateFocusIndex(value);
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

		void setWidthHint(Layout::SizeHint value);

		void setHeightHint(Layout::SizeHint value);

		bool forceOverlay() const {
			return forceOverlay_;
		}	

		void setForceOverlay(bool value) {
			if (forceOverlay_ != value) {
				forceOverlay_ = value;
			}
		}

		virtual void updateVisible(bool value) {
			visible_ = value;
			if (value)
			    onShow(this);
			else
			    onHide(this);
		}

		virtual void updateFocused(bool value) {
			focused_ = value;
			if (value)
				onFocusIn(this);
			else
				onFocusOut(this);
		}

		virtual void updateFocusStop(bool value);
		virtual void updateFocusIndex(unsigned value);

		virtual void updateEnabled(bool value) {
			enabled_ = value;
			// disabled window can;t be focused
			if (!enabled_)
			    setFocused(false);
			if (enabled_)
			    onEnabled(this);
			else 
			    onDisabled(this);
		}

		/** Requests the contents of the clipboard to be sent back to the widget via the paste method. 
		 */
		void requestClipboardContents();

		/** Requests the contents of the selection to be sent back to the widget via the paste method. 
		 */
		void requestSelectionContents();

		void setClipboardContents(std::string const & value);

		void setSelectionContents(std::string const & value);

		virtual void selectionInvalidated() {
			ASSERT(false) << "selectionInvalidated must be implemented at widgets which handle selection";
		}

		/** Whenever user or widget requested clipboard or selection contents is received, the paste method is called with the contents. 
		 */
		virtual void paste(std::string const & contents) {
			MARK_AS_UNUSED(contents);
			// do nothing by default
		}

		virtual void mouseDown(int col, int row, MouseButton button, Key modifiers) {
			if (!focused_ && focusStop_)
			    setFocused(true);
			onMouseUp(this, MouseButtonEvent{ col, row, button, modifiers });
		}

		virtual void mouseUp(int col, int row, MouseButton button, Key modifiers) {
			if (!focused_ && focusStop_)
			    setFocused(true);
			onMouseUp(this, MouseButtonEvent{ col, row, button, modifiers });
		}

		virtual void mouseClick(int col, int row, MouseButton button, Key modifiers) {
			onMouseClick(this, MouseButtonEvent{ col, row, button, modifiers });
		}

		virtual void mouseDoubleClick(int col, int row, MouseButton button, Key modifiers) {
			onMouseDoubleClick(this, MouseButtonEvent{ col, row, button, modifiers });
		}

		virtual void mouseWheel(int col, int row, int by, Key modifiers) {
			if (!focused_ && focusStop_)
			    setFocused(true);
			onMouseWheel(this, MouseWheelEvent{ col, row, by, modifiers });
		}

		virtual void mouseMove(int col, int row, Key modifiers) {
			onMouseMove(this, MouseMoveEvent{ col, row, modifiers });
		}

		virtual void mouseOver() {
			onMouseOver(this);
		}

		virtual void mouseOut() {
			onMouseOut(this);
		}

		virtual void keyChar(helpers::Char c) {
			MARK_AS_UNUSED(c);
		}

		virtual void keyDown(Key k) {
			MARK_AS_UNUSED(k);
		}

		virtual void keyUp(Key k) {
			MARK_AS_UNUSED(k);
		}

		/** Invalidates the widget and request its parent repaint,

		    If the widget is valid, invalidates its visible region and informs its parent that a child was invalidated. If the widget is already not valid, does nothing because the parent has already been notified. 
		 */
		void invalidate();

		/** Invalidates the contents of the widget. 
         
            This invalidates the widget itself and any of its children.
		
		 */
		virtual void invalidateContents() {
			visibleRect_.invalidate();
		}

		/** Updates the position of the widget. 
		
		    Already assumes that the new position is different from the current position. However, in case the requested position is invalid, the widget may adjust the size before setting it. After the size is updated, the control is invalidated and onMove event triggered. 
		 */
		virtual void updatePosition(int x, int y) {
			x_ = x;
			y_ = y;
			invalidate();
			onMove(this);
		}

		/** Updates the size of the widget. 
		
		    Assumes the size is different that current size. However, if the size is invalid, the widget may choose to update the requested width and height accordingly. Invalidates the widget and triggers the onResize event. 
		 */
		virtual void updateSize(int width, int height) {
			ASSERT(width >= 0 && height >= 0);
			width_ = width;
			height_ = height;
			invalidate();
			onResize(this);
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

		virtual void updateOverlay(bool value) {
			overlay_ = value;
		}

		/** Given mouse coordinates, determine the immediate child that is the target of the mouse event. If no such child can be found, returns itself. 
		 */
		virtual Widget * getMouseTarget(int col, int row) {
			ASSERT(visibleRect_.contains(col, row));
			MARK_AS_UNUSED(col);
			MARK_AS_UNUSED(row);
			return this;
		}

		/** Returns the mouse coordinates inside the widget. 
		 */
		Point getMouseCoordinates() const;

	private:

		friend class Layout;
		friend class Container;
		friend class RootWindow;
		friend class Canvas;

		/** Detaches the wdget from its root window. 
		 
		    Informs the root window that the widget is to be detached. 
		 */
        virtual void detachRootWindow();

		/* Parent widget or none. 
		 */
		Container * parent_;

		/* Visible region of the canvas. 
		 */
		Canvas::VisibleRect visibleRect_;

		/* If true, the rectangle of the widget is shared with other widgets, i.e. when the widget is to be repainted, its parent must be repainted instead. */
		bool overlay_;

		/** Forces the overlay to be always true. This is especially useful for controls with transparent backgrounds. */
		bool forceOverlay_;

		/* Visibility */
		bool visible_;

		/** Determines whether the widget is enabled (i.e. can accept user input), or not. */
		bool enabled_;

		/** Determines whether the widget receives keyboard events. */
		bool focused_;

		/** Determines whether the widget can be focused by user's action, usually by the TAB key, and if so, what is the widget's index in the circular list of focusable elements. */
		bool focusStop_;
		unsigned focusIndex_;

		/* Position */
		int x_;
		int y_;

		/* Size */
		int width_;
		int height_;

		Layout::SizeHint widthHint_;
		Layout::SizeHint heightHint_;

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
		using Widget::onMouseOver;
		using Widget::onMouseOut;

		// Methods 

		using Widget::setVisible;
		using Widget::move;
		using Widget::resize;
		using Widget::setWidthHint;
		using Widget::setHeightHint;

	}; // ui::Control

} // namespace ui



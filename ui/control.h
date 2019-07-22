#pragma once
#ifdef FOO

#include "helpers/object.h"
#include "helpers/shapes.h"

#include "canvas.h"

namespace ui {

	/** Determines the size hint for a control. 

	    Specifies size hints when automatic layout is used. 

		- fixed
		- percentage

	 */
	class SizeHint {
	public:
		enum class Kind {
			Auto = 256, 
			Fixed = 512,
			Percentage = 1024
		};

		SizeHint() :
			raw_(static_cast<unsigned>(Kind::Auto)) {
		}

		static SizeHint Fixed() {
			return SizeHint(Kind::Fixed);
		}

		static SizeHint Auto() {
			return SizeHint(Kind::Auto);
		}

		static SizeHint Percentage(unsigned value) {
			ASSERT(value <= 100) << "Size hint percentage too large.";
			return SizeHint(Kind::Percentage, value);
		}

		Kind kind() const {
			return static_cast<Kind>(raw_ & 0xffffff00);
		}

		unsigned value() const {
			ASSERT(kind() == Kind::Percentage);
			return raw_ & 0xff;
		}

		bool operator == (SizeHint const& other) const {
			return raw_ == other.raw_;
		}

		bool operator != (SizeHint const& other) const {
			return raw_ != other.raw_;
		}

		bool operator == (Kind kind) const {
			return this->kind() == kind;
		}

		bool operator != (Kind kind) const {
			return this->kind() != kind;
		}

		operator Kind () {
			return kind();
		}

		
		/** Calculates the appropriate size based on the given values and the size hint. 
		
		    Expects the current value, the value to be used if the the size hint is automatic and the size corresponding to 100 percent. 
		 */
		unsigned calculate(unsigned current, unsigned autoValue, unsigned fullPct) {
			switch (kind()) {
				case Kind::Fixed:
					return current;
				case Kind::Auto:
					return autoValue;
				case Kind::Percentage:
					return fullPct * value() / 100;
				default:
					UNREACHABLE;
			}
		}

	private:

		static unsigned constexpr FIXED = 256;
		static unsigned constexpr AUTO = 512;

		SizeHint(Kind kind, unsigned value = 0) :
			raw_(static_cast<unsigned>(kind) + value) {
		}

		unsigned raw_;
	};

	/** Base class for all UI controls. 




	Key is that parent can be either dynamic (container), or static (composition and explicit drawing methods

	-- base control
	-- container, which contains other controls and maps them automatically

	 */
	class Control : virtual public helpers::Object {
	public:

		Control(int left, int top, unsigned width, unsigned height) :
			parent_(nullptr),
			visibleRegion_(nullptr),
			left_(left),
			top_(top),
			width_(width),
			height_(height),
		    widthHint_(SizeHint::Auto()), 
		    heightHint_(SizeHint::Auto()),
			minWidth_(0),
			maxWidth_(0),
			minHeight_(0),
			maxHeight_(0) {
		}

		Control* parent() const {
			return parent_;
		}

		int top() const {
			return top_;
		}

		int left() const {
			return left_;
		}

		unsigned width() const {
			return width_;
		}

		unsigned height() const {
			return height_;
		}

		SizeHint widthHint() const {
			return widthHint_;
		}

		SizeHint heightHint() const {
			return heightHint_;
		}

		unsigned minWidth() const {
			return minWidth_;
		}

		unsigned maxWidth() const {
			return maxWidth_;
		}

		unsigned minHeight() const {
			return minHeight_;
		}

		unsigned maxHeight() const {
			return maxHeight_;
		}

		void setWidthHint(SizeHint hint) {
			if (widthHint_ != hint) {
				widthHint_ = hint;
				// TODO repaint parent
			}
		}

		void setHeightHint(SizeHint hint) {
			if (heightHint_ != hint) {
				heightHint_ = hint;
				// TODO repaint parent
			}
		}


		/** Resizes the control. 
		 */
		void resize(unsigned width, unsigned height) {
			if (width_ != width || height_ != height) {
				doResize(width, height);
				invalidate();
				if (parent_ != nullptr)
					parent_->doChildGeometryChanged(this);
				repaint();
			}
		}

		void reposition(int left, int top) {
			NOT_IMPLEMENTED;
		}

		/** Triggers the repaint event. 
		 */
		void repaint();

	protected:

		void invalidate() {
			visibleRegion_.invalidate();
		}

		virtual void doRegisterChild(Control* child) {
			MARK_AS_UNUSED(child);
		}

		/** Obtains the visible region. 

		    The default implementation checks if the control has a parent and if it does, triggers the repaint of the parent. This in turn shall update the valid region of the child and then repaint it as well, if necessary. 

			If there is no parent, the control is detached and there is no point in drawing it. 
		 */
		virtual void doGetVisibleRegion() {
			if (parent_ != nullptr)
				parent_->repaint();
		}

		virtual void doResize(unsigned width, unsigned height) {
			width_ = width;
			height_ = height;
		}

		virtual void doPaint(Canvas& canvas) = 0;


		/** Updates the specific child. 

		    Assuming the child's geometry is correct, creates a canvas for the child, 
		 */
		virtual void doUpdateChild(Canvas & canvas, Control * child) {
			ASSERT(child->parent_ == this);
			/* Create the child canvas from our canvas and child's geometry, then update the child's visible region so that next time the child can update itself and finally repaint it. 
			 */
			Canvas childCanvas = Canvas(canvas, child->left_, child->top_, child->width_, child->height_);
			child->visibleRegion_ = childCanvas.visibleRegion_;
			child->doPaint(childCanvas);
		}

		/** Whenever child's position or size changes, it informs its parent using this method. 
		 */
		virtual void doChildGeometryChanged(Control* child) {
			// do nothing in control
		}

	private:

		friend class RootWindow;
		friend class Container;
		friend class Layout;

		/** TODO perhaps trigger the resize and position events? 
		 */
		void forceGeometry(int left, int top, unsigned width, unsigned height) {
			if (width < minWidth_)
				width = minWidth_;
			else if (maxWidth_ != 0 && width > maxWidth_)
				width = maxWidth_;
			if (height < minHeight_)
				height = minHeight_;
			else if (maxHeight_ != 0 && height > maxHeight_)
				height = maxHeight_;
			left_ = left;
			top_ = top;
			width_ = width;
			height_ = height;
		}

		Control * parent_;

		Canvas::VisibleRegion visibleRegion_;

		/** Position of the control, relative to the parent control.
		 */
		int top_;
		int left_;

		/** Dimensions of the control.
		 */
		unsigned width_;
		unsigned height_;

		/** Size hints for automated layouting. 
		 */
		SizeHint widthHint_;
		SizeHint heightHint_;

		/** Minimal and maximal width and height. 
		 */
		unsigned minWidth_;
		unsigned maxWidth_;
		unsigned minHeight_;
		unsigned maxHeight_;


	};
} // namespace ui
#endif
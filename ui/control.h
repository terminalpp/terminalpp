#pragma once

#include "helpers/object.h"
#include "helpers/shapes.h"

#include "canvas.h"
#include "layout.h"

namespace ui {

	/** Base class for all UI controls. 

	Key is that parent can be either dynamic (container), or static (composition and explicit drawing methods

	-- base control
	-- container, which contains other controls and maps them automatically

	 */
	class Control : virtual public helpers::Object {
	public:

		Control(Control* parent, int left, int top, unsigned width, unsigned height) :
			parent_(parent),
			visibleRegion_(nullptr),
			left_(left),
			top_(top),
			width_(width),
			height_(height) {
			if (parent_ != nullptr)
				parent_->doRegisterChild(this);
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

		/** Triggers the repaint event. 
		 */
		void repaint();

	protected:

		Control(unsigned width, unsigned height) :
			parent_(nullptr),
			visibleRegion_(nullptr),
			left_(0),
			top_(0),
			width_(width),
			height_(height) {
		}

		void invalidate() {
			visibleRegion_.invalidate();
		}

		virtual void doRegisterChild(Control* child) {
			MARK_AS_UNUSED(child);
		}

		/** Returns the layout used for children controls, which is None by default. 
		 */
		virtual Layout * doGetLayout() const {
			return Layout::None();
		}

		/** Obtains the visible region. 

		    The default implementation checks if the control has a parent and if it does, triggers the repaint of the parent. This in turn shall update the valid region of the child and then repaint it as well, if necessary. 

			If there is no parent, the control is detached and there is no point in drawing it. 
		 */
		virtual void doGetVisibleRegion() {
			if (parent_ != nullptr)
				parent_->repaint();
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

	private:

		friend class RootWindow;

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



	};
} // namespace ui
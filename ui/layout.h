#pragma once
#ifdef FOO

#include "control.h"

namespace ui {

	class Control;
	class Container;


	/** Defines a layout for children components. 
	 */
	class Layout {
	public:

		/** No layout means that childr components are free to choose their own position and size. 
		 */
		static Layout* None();

		/** In maximized layout, each child's width and height is set to the maximum width and height of the parent. 
		 */
		static Layout* Maximized();

		static Layout* Vertical();

		static Layout* Horizontal();

	protected:

		std::vector<Control*>& childrenOf(Container* container);

		void forceGeometry(Control* child, int left, int top, unsigned width, unsigned height) {
			child->forceGeometry(left, top, width, height);
		}

	private:

		friend class Container;

		virtual void resized(Container* container) = 0;
		virtual void childChanged(Container * container, Control * child) = 0;

		class NoneImplementation;
		class MaximizedImplementation;
		class VerticalImplementation;
		class HorizontalImplementation;


	}; // ui::Layout

	class Layout::NoneImplementation : public Layout {
	private:
		void resized(Container* container) override {
			MARK_AS_UNUSED(container);
		}

		void childChanged(Container* container, Control* control) override {
			MARK_AS_UNUSED(container);
			MARK_AS_UNUSED(control);
		}
	};

	class Layout::MaximizedImplementation : public Layout {
	private:
		void resized(Container* container) override;
		void childChanged(Container* container, Control* control) override;
	};

	class Layout::VerticalImplementation : public Layout {
	private:
		void resized(Container* container) override;
		void childChanged(Container* container, Control* control) override;
	};

	class Layout::HorizontalImplementation : public Layout {
	private:
		void resized(Container* container) override;
		void childChanged(Container* container, Control* control) override;
	};


	inline Layout* Layout::None() {
		static NoneImplementation l;
		return & l;
	}

	inline Layout* Layout::Maximized() {
		static MaximizedImplementation l;
		return &l;
	}
	inline Layout* Layout::Vertical() {
		static VerticalImplementation l;
		return &l;
	}
	inline Layout* Layout::Horizontal() {
		static HorizontalImplementation l;
		return &l;
	}



} // namespace ui
#endif
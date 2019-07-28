#pragma once

#include "vterm/terminal.h"

#include "container.h"

namespace ui {

	typedef vterm::Key Key;

	typedef vterm::MouseButton MouseButton;

	/** The UI window, which is a virtual terminal on which the UI elements are drawn. 

	
	 */
	class RootWindow : public vterm::Terminal, public Container {
	public:

		static unsigned MOUSE_CLICK_MAX_DURATION;
		static unsigned MOUSE_DOUBLE_CLICK_MAX_INTERVAL;

		RootWindow(unsigned width, unsigned height) :
			Terminal(width, height),
			Container(0, 0, width, height),
		    mouseFocus_(nullptr) {
			visibleRegion_ = Canvas::VisibleRegion(this);
		}

		// terminal interface

		void keyDown(Key k) override;
		void keyUp(Key k) override;
		void keyChar(helpers::Char c) override;

		void mouseDown(unsigned col, unsigned row, MouseButton button, Key modifiers) override;
		void mouseUp(unsigned col, unsigned row, MouseButton button, Key modifiers) override;
		void mouseWheel(unsigned col, unsigned row, int by, Key modifiers) override;
		void mouseMove(unsigned col, unsigned row, Key modifiers) override;

		void paste(std::string const& what) override;

		// widget interface

		using Container::addChild;
		using Container::removeChild;
		using Container::setLayout;

	protected:

		friend class Widget;

		// terminal interface

		void doOnResize(unsigned cols, unsigned rows) override;

		/** Triggers the repaint of the terminal by triggering the terminal's onRepaint event to which the renderers subscribe. 
		 */
		void terminalRepaint() {
			helpers::Object::trigger(onRepaint);
		}

		// widget interface

		void invalidateContents() override;

		/** Takes screen coordinates and converts them to the widget's coordinates.

			Returns (-1;-1) if the screen coordinates are outside the current widget.
		 */
		Point screenToWidgetCoordinates(Widget * w, unsigned col, unsigned row) {
			Canvas::VisibleRegion const& wr = w->visibleRegion_;
			if (!wr.isValid() || wr.windowOffset.col > static_cast<int>(col) || wr.windowOffset.row > static_cast<int>(row))
				return Point(-1, -1);
			Point result(
				wr.region.left + (col - wr.windowOffset.col),
				wr.region.top + (row - wr.windowOffset.row)
			);
			ASSERT(result.col >= 0 && result.row >= 0);
			if (result.col >= w->width() || result.row >= w->height())
				return Point(-1, -1);
			return result;
		}

	private:

		Widget* mouseFocusWidget(unsigned col, unsigned row);

		unsigned mouseCol_;
		unsigned mouseRow_;
		Widget* mouseFocus_;
		Widget* mouseClickWidget_;
		MouseButton mouseClickButton_;
		size_t mouseClickStart_;
		size_t mouseDoubleClickPrevious_;



	};


} // namespace uissss
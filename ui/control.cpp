#include "control.h"
#include "root_window.h"

namespace ui {

	void Control::repaint() {
		/* If the control's visible region is valid (i.e. it has not changed its size or position), then we just obtain a lock on the root windows's screen, create the canvas and then call the doPaint method, which is responsible for the actual drawing of the window. */
		if (visibleRegion_.isValid()) {
			vterm::Terminal::ScreenLock screenLock = visibleRegion_.root->lockScreen();
			Canvas canvas(visibleRegion_, *screenLock, width_, height_);
			doPaint(canvas);
			/* If the visible region is not valid, it must be revalidated, which should trigger the repaint when done.
			 */
		} else {
			doGetVisibleRegion();
		}
	}

} // namespace ui
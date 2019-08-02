#include "root_window.h"

namespace ui {

    void RootWindow::invalidateContents() {
        Container::invalidateContents();
        visibleRegion_ = Canvas::VisibleRegion(this);
    }



} // namespace ui
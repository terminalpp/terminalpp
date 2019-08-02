#include "root_window.h"

namespace ui {

    void RootWindow::mouseDown(int col, int row, MouseButton button, Key modifiers) {

    }
    
    void RootWindow::mouseUp(int col, int row, MouseButton button, Key modifiers) {

    }

    void RootWindow::mouseWheel(int col, int row, int by, Key modifiers) {

    }

    void RootWindow::mouseMove(int col, int row, Key modifiers) {

    }



    void RootWindow::invalidateContents() {
        Container::invalidateContents();
        visibleRegion_ = Canvas::VisibleRegion(this);
    }



} // namespace ui
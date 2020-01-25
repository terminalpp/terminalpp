#include "root_window.h"

#include "renderer.h"

namespace ui {


    void Renderer::attach(RootWindow * newRootWindow) {
        ASSERT(rootWindow_ == nullptr);
        ASSERT(newRootWindow->renderer_ == nullptr);
        rootWindow_ = newRootWindow;
        rootWindow_->attachRenderer(this);
        rootWindow_->rendererResized(cols(), rows());
    }

    void Renderer::detach() {
        if (! rootWindow_)
            return;
        rootWindow_->renderer_ = nullptr;
        rootWindow_ = nullptr;
    }


} // namespace ui
#include "canvas.h"
#include "widget.h"
#include "renderer.h"

namespace ui {

    // Renderer ===================================================================================

#ifndef NDEBUG
    std::thread::id Renderer::UIThreadId_;
#endif

    std::deque<std::pair<std::function<void(void)>, Widget*>> Renderer::UserEvents_;
    std::mutex Renderer::M_;
    std::function<void(void)> Renderer::UserEventScheduler_;


    // LocalRenderer ==============================================================================

    size_t LocalRenderer::MouseClickMaxDuration_ = 200;
    size_t LocalRenderer::MouseDoubleClickMaxDistance_ = 200;


} // namespace ui
#include "common.h"
#include "renderer.h"

namespace ui {

#ifndef NDEBUG
std::thread::id UIThreadChecker_::ThreadId() {
    return Renderer::UIThreadId_;
}
#endif

} // namespace ui

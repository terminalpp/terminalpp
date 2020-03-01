#include "common.h"

#include "renderer.h"

namespace ui2 {

#ifndef NDEBUG

    UiThreadChecker_::~UiThreadChecker_() {
        if (renderer_ == nullptr)
            return;
        std::lock_guard<std::mutex> g{renderer_->uiThreadCheckMutex_};
        ASSERT(renderer_->uiThreadChecksDepth_ > 0);
        --renderer_->uiThreadChecksDepth_;
    }

    void UiThreadChecker_::initialize() {
        std::lock_guard<std::mutex> g{renderer_->uiThreadCheckMutex_};
        if (++renderer_->uiThreadChecksDepth_ == 1) 
            renderer_->uiThreadId_ = std::this_thread::get_id();
        else 
            ASSERT(renderer_->uiThreadId_ == std::this_thread::get_id()) << "Multithreaded access to UI elements detected";
    }

#endif

} // namespace ui
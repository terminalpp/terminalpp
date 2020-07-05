#include "widget.h"

#include "renderer.h"

namespace ui3 {

    // Events

    void Renderer::schedule(std::function<void()> event, Widget * widget) {
        {
            std::lock_guard<std::mutex> g{eventsGuard_};
            events_.push_back(std::make_pair(event, widget));
            ++widget->pendingEvents_;
        }
    }

    void Renderer::yieldToUIThread() {
        std::unique_lock<std::mutex> g{yieldGuard_};
        schedule([this](){
            std::lock_guard<std::mutex> g{yieldGuard_};
            yieldCv_.notify_all();
        });
        yieldCv_.wait(g);
    }

    void Renderer::processEvent() {
        std::function<void()> handler;
        {
            std::lock_guard<std::mutex> g{eventsGuard_};
            while (true) {
                if (events_.empty())
                    return;
                auto e = events_.front();
                events_.pop_front();
                if (! e.first)
                    continue;
                if (e.second != nullptr)
                    -- (e.second->pendingEvents_);
                handler = e.first;
                break;
            }
            handler();
        }
    }

    void Renderer::cancelWidgetEvents(Widget * widget) {
        std::lock_guard<std::mutex> g{eventsGuard_};
        if (widget->pendingEvents_ == 0)
            return;
        for (auto & e : events_)
            if (e.second == widget)
                e.first = nullptr;
    }

    // Widget Tree

    void Renderer::setRoot(Widget * value) {
        if (root_ != value) {
            if (root_ != nullptr)
                detachTree(root_);
            root_ = value;
            if (value != nullptr) {
                root_->visibleArea_.attach(this);
                if (root_->rect().size() != size())
                    root_->resize(size());
                else
                    root_->relayout();
            }
        }
    }


    void Renderer::detachWidget(Widget * widget) {
        {
            // detach the visible area
            std::lock_guard<std::mutex> g{widget->rendererGuard_};
            widget->visibleArea_.detach();
        }
        for (Widget * child : widget->children_)
            detachWidget(child);
        widgetDetached(widget);
    }

    // Layouting and painting

    void Renderer::resize(Size const & value) {
        if (buffer_.size() != value) {
            buffer_.resize(value);
            // resize the root widget if any
            if (root_ != nullptr)
                root_->resize(value);
        }
    }

    void Renderer::paint(Widget * widget) {
        if (renderWidget_ == nullptr)
            renderWidget_ = widget;
        else
            renderWidget_ = renderWidget_->commonParentWith(widget);
        ASSERT(renderWidget_ != nullptr);
        // if fps is 0, render immediately, otherwise wait for the renderer to paint
        if (fps_ == 0) 
            paintAndRender();
    }

    void Renderer::paintAndRender() {
        if (renderWidget_ == nullptr)
            return;
        // lock the buffer in priority mode and paint the widget (this also clears the pending repaint flags)
        renderWidget_->paint();
        // render the visible area of the widget, still under the priority lock
        render(buffer_, renderWidget_->visibleArea_.bufferRect());
        renderWidget_ = nullptr;
    }   

    void Renderer::startRenderer() {
        if (renderer_.joinable())
            renderer_.join();
        renderer_ = std::thread([this](){
            while (fps_ != 0) {
                schedule([this](){
                    paintAndRender();
                });
                std::this_thread::sleep_for(std::chrono::milliseconds(1000 / fps_));
            }
        });
    }

} // namespace ui3
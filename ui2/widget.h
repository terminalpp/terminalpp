#pragma once

#include <atomic>

#include "helpers/helpers.h"
#include "helpers/events.h"

#include "common.h"
#include "buffer.h"
#include "canvas.h"
#include "renderer.h"

namespace ui2 {

    class Canvas;

	template<typename P, typename T = Widget>
	using Event = helpers::Event<P, T>;


    /** Base class for widgets. 
     
        A widget can paint itself and can react to user interaction. 

     */
    class Widget {
    public:

        virtual ~Widget() {
            ASSERT(renderer_ == nullptr);
        }

        /** Returns the rectangle occupied by the widget in its parent's contents area. 
         */
        Rect const & rect() const {
            return rect_;
        }

        bool visible() const {
            return visible_;
        }

        /** Triggers the repaint of the widget. [thread-safe]
         */
        // TODO ideally this should do nothing if the repaint has already been scheduled, but did not progress yet
        virtual void repaint() {
            Renderer * r = renderer();
            if (r != nullptr)
                r->repaint(this);
        }

    protected:

        friend class Renderer;

        Widget(int x = 0, int y = 0, int width = 0, int height = 0):
            renderer_{nullptr},
            parent_{nullptr},
            rect_{Rect::FromTopLeftWH(x, y, width, height)},
            overlaid_{false},
            visible_{true} {
        }

        /** Attaches the widget to given parent. 
         
            If the parent has valid renderer attaches the renderer as well.
         */
        virtual void attachTo(Widget * parent) {
            UI_THREAD_CHECK;
            ASSERT(parent_ == nullptr && parent != nullptr);
            parent_ = parent;
            if (parent_->renderer_ != nullptr)
                attachRenderer(parent_->renderer_);
        }

        /** Detaches the widget from its parent. 
         
            If the widget has renderer attached, detaches from the parent first and then detaches from the renderer.
         */
        virtual void detachFrom(Widget * parent) {
            UI_THREAD_CHECK;
            ASSERT(parent_ = parent);
            parent_ = nullptr;
            if (renderer_ != nullptr)
                detachRenderer();
        }

        /** Attaches the widget to specified renderer. 
         
            The renderer must be valid and parent must already be attached if parent exists. 
         */
        virtual void attachRenderer(Renderer * renderer) {
            UI_THREAD_CHECK;
            ASSERT(renderer_ == nullptr && renderer != nullptr);
            ASSERT(parent_ == nullptr || parent_->renderer_ == renderer);
            renderer_.store(renderer);
            // TODO should the renderer be notified that the widget has been attached? 
        }

        /** Detaches the renderer. 
         
            If parent is valid, its renderer must be attached to enforce detachment of all children before the detachment of the parent. 
         */
        virtual void detachRenderer() {
            UI_THREAD_CHECK;
            ASSERT(renderer_ != nullptr);
            ASSERT(parent_ == nullptr || parent_->renderer_ != nullptr);
            renderer_.store(nullptr);
            // TODO the renderer should be notified that a widget has been attached
        }

        /** Returns the parent widget. 
         */
        Widget * parent() const {
            UI_THREAD_CHECK;
            return parent_;
        }

        /** Returns the renderer of the widget. [thread-safe]
         */
        Renderer * renderer() const {
            return renderer_;
        }

        /** Returns the canvas to be used for drawing the contents of the widget. 
         */
        virtual Canvas getContentsCanvas(Canvas & canvas) {
            UI_THREAD_CHECK;
            return canvas;
        }

        /** Paints the widget on the given canvas. 
         
            The canvas is guaranteed to have the width and height of the widget itself. This method *must* be implemented in widget subclasses to actually draw the contents of the widget. 
         */
        virtual void paint(Canvas & canvas) = 0;

        /** Returns true if the widget is overlaid by other widgets. 
         
            Note that this is an implication, i.e. if the widget is not overlaid by other widgets, it can still return true. 
         */
        bool isOverlaid() const {
            UI_THREAD_CHECK;
            return overlaid_;
        }

    private:

        friend class Canvas;

        /** The renderer the widget is attached to. */
        std::atomic<Renderer*> renderer_;

        /** Parent widget, nullptr if none. */
        Widget * parent_;

        /** The rectangle occupied by the widget in its parent's contents area. */
        Rect rect_;

        /** If true, parts of the widget can be overlaid by other widgets and therefore any repaint request of the widget is treated as a repaint request of its parent. */
        bool overlaid_;

        /** \name Visible rectangle properties

         */
        //@{
        Rect visibleRect_;
        Point bufferOffset_;
        //@}

        /** True if the widget should be visible. Widgets that are not visible will never get painted. 
         */
        bool visible_;



    };

} // namespace ui2
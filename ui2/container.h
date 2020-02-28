#pragma once

#include "widget.h"

namespace ui2 {

    class Container : public Widget {
    public:

        /** Adds the given widget as child. 
         
            The widget will appear as the topmost widget in the container. If the widget already exists in the container, it will be moved to the topmost position. 
         */
        virtual void add(Widget * widget) {
            for (auto i = children_.begin(), e = children_.end(); i != e; ++i)
                if (i == widget) {
                    children_.erase(i);
                    children_.push_back(widget);
                    // TODO how to handle invalidations
                }
            // TODO this is not done by far

        }

        /** Remove the widget from the container. 
         */
        virtual void remove(Widget * widget) {
            for (auto i = children_.begin(), e = children_.end(); i != e; ++i)
                if (i == widget) {
                    *i->detachFrom(this);
                    children_.erase(i);
                    return;
                }
            ASSERT(false);
        }

    protected:

        /** Attaches the container to specified renderer. 

            First the container is attached and then all its children are attached as well. 
         */
        void attachRenderer(Renderer * renderer) override {
            UI_THREAD_CHECK;
            Widget::attachRenderer(renderer);
            for (Widget * child : children_)
                child->attachRenderer(renderer);
        }

        /** Detaches the container from its renderer. 
         
            First detaches all children and then detaches itself. 
         */
        void detachRenderer() override {
            UI_THREAD_CHECK;
            for (Widget * child : children_)
                child->detachRenderer();
            Widget::detachRenderer();
        }

        /** Paints the container. 
         
            The default implementation simply obtains the contents canvas and then paints all visible children. 
         */
        void paint(Canvas & canvas) override {
            UI_THREAD_CHECK;
            Canvas childrenCanvas{getContentsCanvas(canvas)};
            for (Widget * child : children_)
                if (child->visible())
                    child->paint(childrenCanvas.clip(child->rect()));
        }

    private:
        std::vector<Widget *> children_;

    };


} // namespace ui
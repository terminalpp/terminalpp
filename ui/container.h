#pragma once

#include "widget.h"

namespace ui {

    /** Container manages its child widgets dynamically. 
     
        A container is a basic widget that manages its children dynamically via a list. Child widgets can be added to, or removed from the container at runtime. The container furthermore provides support for automatic layouting of the children and makes sure that the UI events are propagated to them correctly. 

        TODO the order of elements in the container vector. 
     */
    class Container : public Widget {
    public:

        /** Container destructor also destroys its children. 
         */
        ~Container() override {
            while (!children_.empty()) {
                Widget * w = children_.front();
                remove(w);
                delete w;
            }
        }

        /** Adds the given widget as child. 
         
            The widget will appear as the topmost widget in the container. If the widget already exists in the container, it will be moved to the topmost position. 
         */
        virtual void add(Widget * widget) {
            for (auto i = children_.begin(), e = children_.end(); i != e; ++i)
                if (*i == widget) {
                    children_.erase(i);
                    children_.push_back(widget);
                    // TODO how to handle invalidations
                    NOT_IMPLEMENTED;
                }
            // attach the child widget
            children_.push_back(widget);
            if (widget->parent() != this)
                widget->attachTo(this);
        }

        /** Remove the widget from the container. 
         */
        virtual void remove(Widget * widget) {
            for (auto i = children_.begin(), e = children_.end(); i != e; ++i)
                if (*i == widget) {
                    (*i)->detachFrom(this);
                    children_.erase(i);
                    return;
                }
            ASSERT(false);
        }

    protected:

        /** \name Geometry and Layout
         
         */
        //@{

        /** Change in child's rectangle triggers relayout of the container. 
         */
        void childRectChanged(Widget * child) override {
            MARK_AS_UNUSED(child);
            if (! relayouting_) {
                relayouting_ = true;
                relayout();
            }
        }

        virtual void relayout() {
            // TODO actually relayout the stuff
            relayouting_ = false;    
            repaint();
        }
        //@}

        /** \name Mouse Actions
         */
        //@{

        /** Returns the mouse target. 
         
            Scans the children widgets in their visibility order and if the mouse coordinates are found in the widget, propagates the mouse target request to it. If the mouse is not over any child, returns itself.
         */
        Widget * getMouseTarget(Point coords) override {
            for (auto i = children_.rbegin(), e = children_.rend(); i != e; ++i)
                if ((*i)->visible() && (*i)->rect().contains(coords))
                    return (*i)->getMouseTarget(coords - (*i)->rect().topLeft());
            return this;
        }

        //@}

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
                if (child->visible()) {
                    Canvas childCanvas{childrenCanvas.clip(child->rect())};
                    paintChild(child, childCanvas);
                }
        }

        void paintChild(Widget * child, Canvas & childCanvas) {
            child->visibleRect_ = childCanvas.visibleRect();
            child->paint(childCanvas);
        }

    private:
        std::vector<Widget *> children_;

        bool relayouting_;

    };


} // namespace ui
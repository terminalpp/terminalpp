#pragma once

#include "widget.h"
#include "layout.h"

namespace ui {

    /** Container manages its child widgets dynamically. 
     
        A container is a basic widget that manages its children dynamically via a list. Child widgets can be added to, or removed from the container at runtime. The container furthermore provides support for automatic layouting of the children and makes sure that the UI events are propagated to them correctly. 

        TODO the order of elements in the container vector. 
     */
    class Container : public Widget {
    public:

        Container():
            layout_{Layout::None},
            relayouting_{false} {
        }

        /** Container destructor also destroys its children. 
         */
        ~Container() override {
            while (!children_.empty()) {
                Widget * w = children_.front();
                remove(w);
                delete w;
            }
            if (layout_ != Layout::None)
                delete layout_;
        }

    protected:

        template<template<typename> typename X, typename T>
        friend class TraitBase;

        /** Adds the given widget as child. 
         
            The widget will appear as the topmost widget in the container. If the widget already exists in the container, it will be moved to the topmost position. 
         */
        virtual void add(Widget * widget) {
            for (auto i = children_.begin(), e = children_.end(); i != e; ++i)
                if (*i == widget) {
                    children_.erase(i);
                    break;
                }
            // attach the child widget
            children_.push_back(widget);
            if (widget->parent() != this)
                widget->attachTo(this);
            // relayout the children widgets
            relayout();
        }

        /** Remove the widget from the container. 
         */
        virtual void remove(Widget * widget) {
            for (auto i = children_.begin(), e = children_.end(); i != e; ++i)
                if (*i == widget) {
                    (*i)->detachFrom(this);
                    children_.erase(i);
                    // relayout the children widgets after the child has been removed
                    relayout();
                    return;
                }
            UNREACHABLE;
        }

        /** \name Geometry and Layout
         
         */
        //@{

        /** Returns the layout used by the container. 
         */
        Layout * layout() const {
            return layout_;
        }

        /** Sets the layout for the container. 
         */
        virtual void setLayout(Layout * value) {
            if (layout_ != value) {
                if (layout_ != Layout::None)
                    delete layout_;
                layout_ = value;
                relayout();
            }
        }

        /** Relayout the children widgets when the container has been resized. 
         */
        void resized() override {
            relayout();
            Widget::resized();
        }

        /** Change in child's rectangle triggers relayout of the container. 
         */
        void childChanged(Widget * child) override {
            MARK_AS_UNUSED(child);
            relayout();
        }

/*
        void setOverlay(Overlay value) {
            if (value != overlay()) {
                Widget::setOverlay(value);
                layout_->recalculateOverlay(this);
            }
        }

    */

        void relayout() {
            if (relayouting_)
                return;
            relayouting_ = true;
            // actually relayout the stuff
            layout_->relayout(this);
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
                paintChild(child, childrenCanvas);
        }

    private:

        friend class Layout;

        std::vector<Widget *> children_;

        Layout * layout_;
        bool relayouting_;

    };


} // namespace ui
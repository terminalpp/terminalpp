#pragma once

#include "widget.h"
#include "layout.h"

namespace ui {

    /** Container manages its child widgets dynamically. 
     
        A container is a basic widget that manages its children dynamically via a list. Child widgets can be added to, or removed from the container at runtime. The container furthermore provides support for automatic layouting of the children and makes sure that the UI events are propagated to them correctly. 

        ## Adding and Removing Child Widgets

        To add or remove child widgets, the add() and remove() methods must be used. Apart from adding/removing the widget to the list of children they also deal with the necessary bookkeeping such as registering the children with own renderer. 

        ## Painting

        Container's paint method obtains the contents canvas and then paints all the children on it. The contents canvas is the canvas of the container by default, but can be updated by reimplementing the scrollSize() and scrollOffset() methods for scrollable containers. 

        ## Geometry & Layouts

        Each container provides a layout() which is a class responsible for resizing and moving its children when the container itself is resized. Relayouting a container (calculateLayout() method) simply calls the layout object which then updates the geometry of all of its childern. For more details see the Layout class. 

        If a container is autosized, then its size is updated so that all of its children under current layout fit in it. This behavior is implemented in the calculateAutoSize() method. 

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
            if (layout_ != Layout::None)
                delete layout_;
        }

    protected:

        template<template<typename> typename X, typename T>
        friend class TraitBase;

        template<typename T>
        friend class Modal;

        Container(Layout * layout = Layout::None):
            layout_{layout},
            layoutScheduled_{false} {
        }


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
            // make sure the attached widget will be relayouted even if its size won't change (see Layout::resizeChild())
            widget->pendingRelayout_ = true;
            // relayout the container
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

        /** \name Keyboard Input
         */

        //@{
        Widget * getNextFocusableWidget(Widget * current) override {
            if (enabled()) {
                if (current == nullptr && focusable())
                    return this;
                ASSERT(current == this || current == nullptr || current->parent() == this);
                if (current == this)
                    current = nullptr;
                // see if there is a next child that can be focused
                Widget * next = getNextFocusableChild(current);
                if (next != nullptr)
                    return next;
            }
            // if not the container, nor any of its children can be the next widget, try the parent
            return (parent_ == nullptr) ? nullptr : parent_->getNextFocusableWidget(this);
        }

        /** Returns next focusable child. 
         */
        Widget * getNextFocusableChild(Widget * current) {
            // determine the next element to focus, starting from the current element which we do by setting current to nullptr if we see it and returning the child if current is nullptr
            for (Widget * child : children_) {
                if (current == nullptr) {
                    Widget * result = child->getNextFocusableWidget(nullptr);
                    if (result != nullptr)
                        return result;
                } else if (current == child) {
                    current = nullptr;
                }
            }
            return nullptr;
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

        /** Returns the layout used by the container. 
         */
        Layout * layout() const {
            return layout_;
        }

        /** Sets the layout for the container. 
         */
        virtual void setLayout(Layout * value) {
            ASSERT(value != nullptr);
            ASSERT(value->container_ == nullptr);
            if (layout_ != value) {
                if (layout_ != Layout::None)
                    delete layout_;
                layout_ = value;
                if (layout_ != Layout::None)
                    layout_->container_ = this;
                relayout();
            }
        }

        /** Returns the contents canvas size.
         */
        virtual Size scrollSize() const {
            return Size{width(), height()};
        } 

        /** Returns the scroll offset. 
         */
        virtual Point scrollOffset() const {
            return Point{0,0};
        }

        void paint(Canvas & canvas) override {
            // paint the widget itself
            Widget::paint(canvas);
            // get the children canvas and paint the children
            Canvas childrenCanvas{canvas.resize(scrollSize()).offset(scrollOffset())};
            for (Widget * child : children_) {
                Canvas childCanvas{childrenCanvas.clip(child->rect())};
                paintChild(child, childCanvas);
            }
        }

        /** Calculates the layout of the container. 
         
         */
        void calculateLayout() override {
            if (! children_.empty()) {
                Size size = scrollSize();
                layout_->relayout(this, size);
            }
            Widget::calculateLayout();
        }

        /** Calculates the autosize of the container. 
         
            An autosized container must simply be big enough to fit all of its children in its client canvas. 
         */
        Size calculateAutoSize() override {
            UI_THREAD_CHECK;
            Rect r{Rect::Empty()};
            for (Widget * child : children_) {
                if (!child->visible())
                    continue;
                r = r | child->rect_;
            }
            return Size{r.width(), r.height()};
        }

        std::vector<Widget *> children_;

    private:

        friend class Layout;

        Layout * layout_;
        bool layoutScheduled_;

    }; // ui::Container


    class PublicContainer : public Container {
    public:
        PublicContainer(Layout * layout = Layout::None):
            Container{layout} {
        }

        using Container::layout;
        using Container::setLayout;
        using Container::add;
        using Container::remove;
        using Container::setWidthHint;
        using Container::setHeightHint;

        std::vector<Widget *> const & children() const {
            return children_;
        }
    }; // ui::PublicContainer


} // namespace ui
#include <algorithm>
#include <unordered_set>

#include "renderer.h"

#include "widget.h"

namespace ui3 {

    // ============================================================================================
    // Widget Tree

    bool Widget::isDominatedBy(Widget const * widget) const {
        Widget const * x = this;
        while (x != nullptr) {
            if (x == widget)
                return true;
            x = x->parent_;
        }
        return false;
    }

    Widget * Widget::commonParentWith(Widget const * other) const {
        std::unordered_set<Widget const*> parents;
        for (Widget const * w = other; w != nullptr; w = w->parent_)
            parents.insert(w);
        for (Widget * w = const_cast<Widget*>(this); w != nullptr; w = w->parent_)
            if (parents.find(w) != parents.end())
                return w;
        return nullptr;
    }

    /** Attaching widget that is already a child has the effect of bringing it to front, attaching a widget that is already attached to a different widget is not supported.  
     */
    void Widget::attach(Widget * child) {
        if (child->parent() == this) {
            auto i = std::find(children_.begin(), children_.end(), child);
            ASSERT(i != children_.end());
            children_.erase(i);
        } else {
            ASSERT(child->parent() == nullptr);
        }
        children_.push_front(child);
        relayout();
    }

    /** Attaching widget that is already a child has the effect of bringing it to back. 
     */
    void Widget::attachBack(Widget * child) {
        if (child->parent() == this) {
            auto i = std::find(children_.begin(), children_.end(), child);
            ASSERT(i != children_.end());
            children_.erase(i);
        } else {
            ASSERT(child->parent() == nullptr);
        }
        children_.push_back(child);
        relayout();
    }

    void Widget::detach(Widget * child) {
        ASSERT(child->parent() == this);
        // detach from the renderer first
        if (visibleArea_.attached())
            visibleArea_.renderer()->detachTree(child);
        // then delete from own children
        auto i = std::find(children_.begin(), children_.end(), child);
        ASSERT(i != children_.end());
        children_.erase(i);
        // and finally relayout itself
        relayout();
    }

    // ============================================================================================
    // Layouting

    /** TODO what should move of root widget do? 
        TODO when should the events for move & resize be triggered and what is their contract? 
     */
    void Widget::move(Point const & topLeft) {
        // don't do anything if no-op
        if (rect_.topLeft() == topLeft)
            return;
        rect_.move(topLeft);
        // tell parent to relayout
        if (parent_ != nullptr) {
            parent_->relayout();
        } else {
            ASSERT(! visibleArea_.attached()) << "Root widget does not support moving";
        }
        // finally, trigger the onMove event in case parent's relayout did not change own values
        if (rect_.topLeft() == topLeft) {
            NOT_IMPLEMENTED;
        }
    }

    void Widget::resize(Size const & size) {
        // don't do anything if no-op
        if (rect_.size() == size) 
            return;
        rect_.resize(size);
        // tell parent to relayout and mark own layput as pending to be sure parent triggers it, if we have no parent, relayout itself (root widget or unattached)
        if (parent_ != nullptr) {
            pendingRelayout_ = true;
            parent_->relayout();
        } else {
            relayout();
        }
        // finally trigger the on resize event in case the above relayouts did not change the size themselves
        if (rect_.size() == size) {
            NOT_IMPLEMENTED;
        }
    }

    void Widget::relayout() {
        // don't do anything if already relayouting (this silences the move & resize updates from the child widgets), however set the pending relayout to true
        if (relayouting_) {
            pendingRelayout_ = true;
            return;
        }
        // set the relayout in progress flag and clear any pending relayouts as we are doing them now
        relayouting_ = true;
        while (true) {
            // relayout the children, this calculates their sizes and positions and sets their pending relayouts
            layout_->layout(this);
            pendingRelayout_ = false;
            // now relayout any pending children, if these resize themselves while being relayouted, they will call parent's relayout which would flip the pendingRelayout_ to true
            for (Widget * child : children_)
                if (child->pendingRelayout_)
                    child->relayout();
            // if any of the pending children relayouts triggered relayout in parent, the flag tells us and we need to relayout everything
            if (pendingRelayout_)
                continue;
            // children have been adjusted, it is time to adjust ourselves and see if there has been change or not
            Size size = getAutosizeHint();
            if (size != rect_.size()) {
                // we are done with layouting
                relayouting_ = false;
                pendingRelayout_ = false;
                // resize to the provided size - this either triggers parent relayout, or own relayout, which will take precedence over this one
                resize(size);
                return;
            }
            // own size and layout are valid, we are done relayouting, calculate overlays
            layout_->calculateOverlay(this);
            break;
        } 
        // own layout is valid, if we are root of the relayouting subtree (i.e. parent is not relayouting) we must update the visible areas. If we are root element, we must relayout too
        if ((parent_ != nullptr && ! parent_->relayouting_) || isRootWidget())
            updateVisibleArea();
        // relayouting is done
        relayouting_ = false;
    }

    /** If the widget has normal parent, its contents visible are is used. If the parent is not attached, or non-existent, no visible areas are updated. If the widget is root widget, then renderer's visible area is used. 
     */
    void Widget::updateVisibleArea() {
        if (isRootWidget())
            updateVisibleArea(visibleArea_.renderer()->visibleArea());
        else if (parent_ != nullptr && parent_->visibleArea_.attached())
            updateVisibleArea(parent_->getContentsVisibleArea());
        // otherwise do nothing (the widget is not attached and neither is its parent)
        ASSERT(! visibleArea_.attached());
    }

    // ============================================================================================
    // Painting

    void Widget::repaint() {
        // if there is already pending repaint, don't do anything
        if (pendingRepaint_.test_and_set())
            return;
        // propagate the paint event through parents so that they can decide to actually repaint themselves instead, if the repaint is allowed, instruct the renderer to repaint
        if (parent_ == nullptr || parent_->allowRepaintRequest(this)) {
            ASSERT(renderer() != nullptr);
            renderer()->paint(this);
        }
    }

    bool Widget::allowRepaintRequest(Widget * immediateChild) {
        // if there is already a repaint requested on the parent, the child's repaint can be ignored as it will be repainted when the parent does
        if (pendingRepaint_.test_and_set()) {
            return false;
        }
        // if the child from which the request comes is overlaid, then block the request and repaint itself instead
        if (immediateChild->overlaid_) {
            repaint();
            return false;
        }
        // otherwise defer to own parent, or allow if root element
        if (parent_ != nullptr)
            return parent_->allowRepaintRequest(this);
        else
            return true;    
    }



} // namespace ui3
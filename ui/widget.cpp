#include <algorithm>
#include <unordered_set>

#include "renderer.h"

#include "widget.h"

namespace ui {

    // ============================================================================================

    void Widget::schedule(std::function<void()> event) {
        std::lock_guard<std::mutex> g{rendererGuard_};
        if (visibleArea_.renderer() != nullptr)
            visibleArea_.renderer()->schedule(event, this);
    }

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
        child->parent_ = this;
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
        child->parent_ = this;
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
        // tell parent to relayout (which may change the move coordinates, or does nothing if already relayouting, i.e. triggered by relayout of parent to begin with)
        if (parent_ != nullptr) {
            parent_->relayout();
        } else {
            ASSERT(! visibleArea_.attached()) << "Root widget does not support moving";
        }
        // finally, trigger the onMove event in case parent's relayout did not change own values
        if (rect_.topLeft() == topLeft) {
            //NOT_IMPLEMENTED;
        }
    }

    void Widget::resize(Size const & size) {
        // don't do anything if no-op
        if (rect_.size() == size) 
            return;
        // update the size and relayout the widget itself, which updates its contents to the new size and if the widget is autosized and this would change, triggers new resize event. To limit extra repaints, if there is a parent that will be relayouted afterwards, the its relayouting flag is artificially set up when relayouting itself so that the own relayout won't trigger visible area update and repaint as these would be done after the parent finishes
        rect_.resize(size);
        if (parent_ == nullptr || parent_->relayouting_) {
            relayout();
        } else {
            parent_->relayouting_ = true;
            relayout();
            parent_->relayouting_ = false;
            if (rect_.size() != size)
                return;
            parent_->relayout();
        }
        // finally trigger the on resize event in case the above relayouts did not change the size themselves
        //NOT_IMPLEMENTED;
    }
    /**
        # Example Scenarios

        Without any autosizing the layout process with a parent and child proceeds in the following way:

        - (p1) parent's relayout() is invoked, sets layouting_ to true to indicate relayout in progress
        - (p2) parent relayouts its contents, this calls move() and resize() of the child, which would trigger relayout of the parent (but since layouting is in progress, these are no-ops, but they fire the respective events), calling resize() on the child sets its pendingRelayout_ flag
        - (p3) parent sets pendingRelayout_ to false
        - (p4) parent calls relayout() on any children than have pendingRelayout_ flag on (there is no autosizing so these will not re-invoke parents' relayout() method)
        - (c1) child's relayout() is invoked, sets the layouting_ flag
        - (c2-4) identical to **p2-4**
        - (c5) child checks pendingRelayout_, which is  (i.e. no resizes of its children in **c4**, so the layout is valid)
        - (c6) child checks its autosize, but no change, so layout is valid
        - (c7) 
        - (c6) child checks pendingRelayout_ (if there was any that means relayouting its children )
        - (p5) parent checks its getAutosizeHint() method, which in the absence of autosizing returns current size, so no resize of parent will happen
        - (p6) parent  


        With widget whose child autosizes, such as a label that has its width determined by layout, but height determined by the text itself, and with the parent widget being autosized too (height to be that of its children), the following happens:

        - (p1) parent's relayout is invoked, sets layouting_) to true to indicate relayout in progress
        - (p2) parent relayouts its child widgets using its layout
        - (pl1) the layout determines that child's height is autosized and will not update it, but updates width according to the layout, calling resize() on the child
        - (cr1) the resize calls relayout of parent (since parent relayouts, this does nothing )


        Without autosize, for top widget (i.e. resize triggered by user) is simple:

        1) set layouting_ to true
        2) relayout itself (resize & move children, resized children set their pendingRelayout_ flags)
        3) set pendingRelayout_ to false
        4) relayout children that have pending relayouts (none of these change their size, so they will not attempt to trigger relayout in parent)
        5) since the widget itself does not do autosize, it is not resized itself, so no parent relayout happens
        6) since there were no parent relayouts triggered in 4), we are done with relayouting
        7) parent is not relayouting itself (i.e. we are the root), so set layouting_ to false
        8) our size and children's layout are correct (getAutosizeHint() returns current size)
        9) update visible areas transitively
        10) repaint

        From child's point of view, whose resize was triggered by parent relayout, the following happens:

        1) set layouting_ to true
        2-6) like previous example
        7) parent is relayouting (our relayout was triggered from its 4), set layouting_ to false and don't do anything, the root of relayout will do the visible area update & repaint

        If the child has autoresize property *and* gets resized during the layout, then the following happens:

        1-3) on parent are identical as in the first case
        then child gets relayouted, which in its stem 

        - then do child that has autoresize
        - and finally parent that has autoresize

    
    */
    void Widget::relayout() {
        // don't do anything if already relayouting (this silences the move & resize updates from the child widgets)
        if (relayouting_) {
            //pendingRelayout_ = true;
            return;
        }
        // set the relayout in progress flag and clear any pending relayouts as we are doing them now
        relayouting_ = true;
        //while (true) {
            // relayout the children, this calculates their sizes and positions and sets their pending relayouts
            layout_->layout(this);
            //pendingRelayout_ = false;
            // now relayout any pending children, if these resize themselves while being relayouted, they will call parent's relayout which would flip the pendingRelayout_ to true
            //for (Widget * child : children_)
            //    if (child->pendingRelayout_)
            //        child->relayout();
            // if any of the pending children relayouts triggered relayout in parent, the flag tells us and we need to relayout everything
            //if (pendingRelayout_)
            //    continue;
            // children have been adjusted, it is time to adjust ourselves and see if there has been change or not
            Size size = getAutosizeHint();
            if (size != rect_.size()) {
                // we are done with layouting
                relayouting_ = false;
                //pendingRelayout_ = false;
                // resize to the provided size - this either triggers parent relayout, or own relayout, which will take precedence over this one
                resize(size);
                return;
            }
            // own size and layout are valid, we are done relayouting, calculate overlays
            layout_->calculateOverlay(this);
        //    break;
        //} 
        // own layout is valid, if we are root of the relayouting subtree (i.e. parent is not relayouting) we must update the visible areas and repaint. If we are root element, we must relayout too
        if ((parent_ != nullptr && ! parent_->relayouting_) || isRootWidget()) {
            updateVisibleArea();
            repaint();
        }
        // relayouting is done
        relayouting_ = false;
    }

    /** If the widget has normal parent, its contents visible area is used. If the parent is not attached, or non-existent, no visible areas are updated. If the widget is root widget, then renderer's visible area is used. 
     */
    void Widget::updateVisibleArea() {
        if (isRootWidget()) 
            updateVisibleArea(renderer()->visibleArea());
        else if (parent_ != nullptr && parent_->visibleArea_.attached()) 
            updateVisibleArea(parent_->visibleArea_);
        else 
            // otherwise do nothing (the widget is not attached and neither is its parent)
            ASSERT(! visibleArea_.attached());
    }

    void Widget::updateVisibleArea(VisibleArea const & parentArea) {
        ASSERT(parentArea.attached());
        visibleArea_ = parentArea.clip(rect_).offset(scrollOffset_);
        for (Widget * child : children_)
            child->updateVisibleArea(visibleArea_);
    }


    // ============================================================================================
    // Painting

    void Widget::repaint() {
        if (parent_ == nullptr || background_.opaque()) {
            // if there is already pending repaint, don't do anything
            if (pendingRepaint_ == true)
                return;
            pendingRepaint_ = true;
            // propagate the paint event through parents so that they can decide to actually repaint themselves instead, if the repaint is allowed, instruct the renderer to repaint
            if (parent_ == nullptr || parent_->allowRepaintRequest(this)) {
                ASSERT(renderer() != nullptr);
                renderer()->paint(this);
            }
        } else {
            // delegate to parent if background is transparent
            // we already checked that parent is not null
            parent_->repaint();
        }
    }

    bool Widget::allowRepaintRequest(Widget * immediateChild) {
        // if there is already a repaint requested on the parent, the child's repaint can be ignored as it will be repainted when the parent does
        if (pendingRepaint_ == true) {
            return false;
        }
        // if the child from which the request comes is overlaid, then block the request and repaint itself instead
        // the same if there is border on this widget
        // TODO the border only needs top be repainted if the widget touches it
        if (immediateChild->overlaid_ || ! border_.empty()) {
            repaint();
            return false;
        }
        // otherwise defer to own parent, or allow if root element
        if (parent_ != nullptr)
            return parent_->allowRepaintRequest(this);
        else
            return true;    
    }

    // Generic Input

    void Widget::setEnabled(bool value) {
        if (enabled_ != value) {
            enabled_ = value;
            if (enabled_ == false && focused())
                defocus();
            repaint();
        }
    }

    // Keyboard

    bool Widget::focused() const {
        return renderer() != nullptr && renderer()->keyboardFocus() == this;
    }

    void Widget::focus() {
        if (focused() || renderer() == nullptr)
            return;
        renderer()->setKeyboardFocus(this);
    }

    void Widget::defocus() {
        if (! focused())
            return;
        renderer()->setKeyboardFocus(renderer()->nextKeyboardFocus());
        if (focused())
            renderer()->setKeyboardFocus(nullptr);
    }

    void Widget::setFocusable(bool value) {
        if (focusable_ != value) {
            focusable_ = value;
            if (! focusable_ && focused())
                defocus();
        }
    }

    // Selection

    void Widget::requestClipboardPaste() {
        Renderer * r = renderer();
        if (r != nullptr)
            r->requestClipboard(this);
    }

    void Widget::requestSelectionPaste() {
        Renderer * r = renderer();
        if (r != nullptr)
            r->requestSelection(this);
    }

} // namespace ui
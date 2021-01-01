#include <algorithm>
#include <unordered_set>

#include "renderer.h"

#include "widget.h"

namespace ui {

    // ============================================================================================

    void Widget::schedule(std::function<void()> event) {
        std::lock_guard<std::mutex> g{rendererGuard_};
        if (renderer_ != nullptr)
            renderer_->schedule(event, this);
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
        children_.push_back(child);
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
        children_.push_front(child);
        child->parent_ = this;
        relayout();
    }

    void Widget::detach(Widget * child) {
        ASSERT(child->parent() == this);
        // delete from own children first (so that the renderer already sees the correct tree while the widget is detached if needed for things such as focus changes)
        auto i = std::find(children_.begin(), children_.end(), child);
        ASSERT(i != children_.end());
        children_.erase(i);
        // then detach from the renderer first
        if (renderer_ != nullptr)
            renderer_->detachTree(child);
        // and finally relayout itself
        relayout();
    }

    Widget * Widget::nextWidget(std::function<bool(Widget*)> cond, Widget * last, bool checkParent) {
        // if last element is nullptr and we are focusable return itself
        if (last == nullptr && (! cond || cond(this)))
            return this;
        // if not us, try children, if last was widget set last to nullptr, otherwise set last to nullptr when the last widget is found in the children so that next widget will be examined
        if (! cond || cond(this)) {
            if (last == this)
                last = nullptr;
            for (Widget * w : children_) {
                if (last == nullptr) {
                    // check the child, not recursing to parent if not found, since if not found we can check the next sibling here
                    Widget * result = w->nextWidget(cond, nullptr, false);
                    if (result != nullptr)
                        return result;
                } else if (last == w) {
                    last = nullptr;
                }
            }
        }
        // not found in children, try siblings in parent starting after itself
        if (checkParent && parent_ != nullptr)
            return parent_->nextWidget(cond, this, true);
        else
            return nullptr;
    }

    Widget * Widget::prevWidget(std::function<bool(Widget*)> cond, Widget * last, bool checkParent) {
        // reverse iterate the children to see if last is one of them in which case return the one before it
        for (size_t i = children_.size() - 1, e = children_.size(); i < e; --i) {
            Widget * w = children_[i];
            if (last == nullptr) {
                Widget * result = w->prevWidget(cond, nullptr, false);
                if (result != nullptr)
                    return result;
            } else if (last == w) {
                last = nullptr;
            }
        }
        // if last != this, return itself if focusable
        if (last != this && (! cond || cond(this)))
            return this;
        // otherwise recurse to parent, if possible
        if (checkParent && parent_ != nullptr)
            return parent_->prevWidget(cond, this, true);
        else
            return nullptr;
    }
    
    /** The argument is not used, but is kept to signify that the method should only be called from contexts where the widget's canvas exists. 
     */
    Canvas Widget::contentsCanvas(Canvas & from) const {
        MARK_AS_UNUSED(from);
        return Canvas{renderer_->buffer_, visibleArea_.offset(scrollOffset_), contentsSize()};
    }

    /** If the widget has normal parent, its contents visible area is used. If the parent is not attached, or non-existent, no visible areas are updated. If the widget is root widget, then renderer's visible area is used. 
     */
    void Widget::updateVisibleArea() {
        if (isRootWidget()) 
            updateVisibleArea(renderer()->visibleArea(), renderer_);
        else if (parent_ != nullptr && parent_->renderer_ != nullptr) 
            updateVisibleArea(parent_->visibleArea_.offset(parent_->scrollOffset_), renderer_);
        else 
            // otherwise do nothing (the widget is not attached and neither is its parent)
            ASSERT(renderer_ == nullptr);
    }

    void Widget::updateVisibleArea(VisibleArea const & parentArea, Renderer * renderer) {
        renderer_ = renderer;
        visibleArea_ = parentArea.clip(rect_);
        for (Widget * child : children_)
            child->updateVisibleArea(visibleArea_.offset(scrollOffset_), renderer);
    }


    // ============================================================================================
    // Painting

    void Widget::repaint() {
        UI_THREAD_ONLY;
        // check if the repaint is allowed (i.e. if any of the parent widgets steals the repaint for itself, or deems the repaint redundant)
        if (parent_ != nullptr && ! parent_->allowRepaintRequest(this))
            return;
        // repaint the widget via the renderer (so that the renderer can determine things like FPS, etc.)
        if (renderer_ != nullptr)
            renderer_->paint(this); 
    }

    void Widget::paint() {
        UI_THREAD_ONLY;
        // invalidate the repaint request
        requests_.fetch_and(~REQUEST_REPAINT);
        Canvas canvas{renderer_->buffer_, visibleArea_, size()};
        // paint the background first
        canvas.setBg(background_);
        canvas.fill(canvas.rect());
        // now paint whatever the widget contents is
        paint(canvas);
        // paint the border now that the widget has been painted
        canvas.setBorder(canvas.rect(), border());
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

    // Mouse

    bool Widget::mouseFocused() const {
        return renderer() != nullptr && renderer()->mouseFocus() == this;
    }

    void Widget::setMouseCursor(MouseCursor cursor) {
        if (mouseCursor_ != cursor) {
            mouseCursor_ = cursor;
            if (mouseFocused())
                renderer_->setMouseCursor(mouseCursor_);
        }
    }

    void Widget::mouseIn(VoidEvent::Payload & e) {
        renderer_->setMouseCursor(mouseCursor_);
        onMouseIn(e, this);
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
        Widget * w = renderer()->nextKeyboardFocus();
        ASSERT(w != nullptr) << "At least current widget shoulc be returned again";
        if (w == this)
            renderer()->setKeyboardFocus(nullptr); // if we are last focusable widget, clear the focus
        else
            renderer()->setKeyboardFocus(w);
    }

    void Widget::setFocusable(bool value) {
        if (focusable_ != value) {
            focusable_ = value;
            if (! focusable_ && focused())
                defocus();
        }
    }

    // Selection

    void Widget::setClipboard(std::string const & contents) {
        Renderer * r = renderer();
        if (r != nullptr)
            r->setClipboard(contents);
    }

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
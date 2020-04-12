#include "widget.h"
#include "container.h"
#include "layout.h"

namespace ui {

    namespace {

        /** No layout, where any of the layouting decissions are forwarded to the children widgets via their relayout() method. 
         */
        class NoneImpl : public Layout {
        protected:
            void relayout(Container* container, Canvas const& clientCanvas) override {
                int w = clientCanvas.width();
                int h = clientCanvas.height();
                for (Widget* child : childrenOf(container))
                    relayoutWidget(child, w, h);
            }
        }; // NoneImpl

        /** Layout which maximizes the children widgets to fill the entire area of the parent container. 

            If the children cannot be maximized (i.e. their size hints prevent that, or their resize update sets different values, the widget is centered with its size. 
        */
        class MaximizedImpl : public Layout {
        protected:
            void relayout(Container* container, Canvas const& clientCanvas) override {
                int w = clientCanvas.width();
                int h = clientCanvas.height();
                for (Widget* child : childrenOf(container))
                    if (child->visible())
                        layout(container, child, w, h);
            }

            /** Layouts the child according to its size hints and positions it in the center of the parent's canvas.
             */
            void layout(Container* container, Widget* child, int maxWidth, int maxHeight) {
                int w = maxWidth;
                if (child->widthHint().isFixed())
                    w = child->width();
                else if (child->widthHint().isPercentage())
                    w = maxWidth * child->widthHint().pct() / 100;
                int h = maxHeight;
                if (child->heightHint().isFixed())
                    h = child->height();
                else if (child->heightHint().isPercentage())
                    h = maxHeight * child->heightHint().pct() / 100;
                int l = (w == maxWidth) ? 0 : ((maxWidth - w) / 2);
                int t = (h == maxHeight) ? 0 : ((maxHeight - h) / 2);
                setChildGeometry(container, child, l, t, w, h);
                // if the child altered its size, we must reposition it
                if (child->width() != w || child->height() != h) {
                    w = child->width();
                    h = child->height();
                    l = ((maxWidth - w) / 2);
                    t = ((maxHeight - h) / 2);
                    setChildGeometry(container, child, l, t, w, h);
                }
            }
        }; // MaximizedImpl

        class HorizontalImpl : public Layout {
        protected:
            /** None of the children are overlaid.
             */
            void calculateOverlay(Container* container, bool initialOverlay) override {
                for (Widget* child : childrenOf(container))
                    setOverlayOf(child, initialOverlay);
            }

            void layout(Container * container, Widget* child, int top, int maxWidth, int height) {
                if (child->widthHint().isFixed()) 
                    setChildGeometry(container, child, 0, top, child->width(), height);
                else if (child->widthHint().isAuto()) 
                    setChildGeometry(container, child, 0, top, maxWidth, height);
                else 
                    setChildGeometry(container, child, 0, top, maxWidth * child->widthHint().pct() / 100, height);
            }

            /** Sets the sizes for all visible children of the specified element. 

                This leaves the relayout method with only the task to actually position the elements according to the layout rules.  
             */
            bool setChildrenSizes(Container * container, Canvas const & clientCanvas) {
                int w = clientCanvas.width();
                std::vector<Widget*> const& children = childrenOf(container);
                // first determine the number of visible children to layout, the total height occupied by children with fixed height and number of children with height hint set to auto
                int visibleChildren = 0;
                int autoChildren = 0;
                int fixedHeight = 0;
                for (Widget* child : children)
                    if (child->visible()) {
                        ++visibleChildren;
                        if (child->heightHint().isFixed()) {
                            fixedHeight += child->height();
                            layout(container, child, child->y(), w, child->height());
                        } else if (child->heightHint().isAuto()) {
                            ++autoChildren;
                        } 
                    }
                // nothing to layout if there are no kids
                if (visibleChildren == 0)
                    return false;
                // set the width and height of the percentage wanting children
                int totalHeight = clientCanvas.height();
                int availableHeight = std::max(0, totalHeight - fixedHeight);
                for (Widget * child : children)
                    if (child->heightHint().isPercentage() && child->visible()) {
                        // determine the height 
                        int h = (totalHeight - fixedHeight) * child->heightHint().pct() / 100;
                        layout(container, child, child->y(), w, h);
                        // this is important because the height might have been adjusted by the widget itself when it was set by the layout
                        availableHeight -= child->height();
                    }
                // now we know the available height for the auto widgets
                if (autoChildren != 0) {
                    int h = availableHeight / autoChildren;
                    for (Widget* child : children) {
                        if (child->heightHint().isAuto() && child->visible()) {
                            if (--autoChildren == 0)
                                h = availableHeight;
                            layout(container, child, child->y(), w, h);
                            availableHeight -= child->height();
                        }
                    }
                }
                return true;
            }

        }; // HorizontalImpl

        /** Layouts all the children as horizontally stacked bars. 

            Does not resize widgets with fixed-size hint, all percentage hints are relative to the total available height - the fixed widget's height being 100%. The autosized widgets will each get equal share of the remaining available height. The last autosized widget fills up the rest of the parent's container. 

            Unless hinted otherwise, width of all children will be set to the width of the parent's canvas.
        */
        class HorizontalTopImpl : public HorizontalImpl {
        protected:
            void relayout(Container* container, Canvas const& clientCanvas) override {
                if (! setChildrenSizes(container, clientCanvas))
                    return; // nothing to layout
                // finally, when the sizes are right, we must reposition the elements
                std::vector<Widget*> const& children = childrenOf(container);
                int top = 0;
                for (Widget* child : children) {
                    if (child->visible()) {
                        setChildGeometry(container, child, 0, top, child->width(), child->height());
                        top += child->height();
                    }
                }
            }

        }; // HorizontalTopImpl

        class HorizontalBottomImpl : public HorizontalImpl {
        protected:
            void relayout(Container* container, Canvas const& clientCanvas) override {
                if (! setChildrenSizes(container, clientCanvas))
                    return; // nothing to layout
                // finally, when the sizes are right, we must reposition the elements
                std::vector<Widget*> const& children = childrenOf(container);
                int bottom = clientCanvas.height();
                for (Widget* child : children) {
                    if (child->visible()) {
                        setChildGeometry(container, child, 0, bottom - child->height(), child->width(), child->height());
                        bottom -= child->height();
                    }
                }
            }

        }; // HorizontalBottomImpl

        /** Layouts all the children as vertically stacked bars.

            Does not resize widgets with fixed-size hint, all percentage hints are relative to the total available width - the fixed widget's width being 100%. The autosized widgets will each get equal share of the remaining available width. The last autosized widget fills up the rest of the parent's container.

            Unless hinted otherwise, height of all children will be set to the height of the parent's canvas.
        */
        class VerticalImpl : public Layout {
        protected:
            void relayout(Container* container, Canvas const& clientCanvas) override {
                int h = clientCanvas.height();
                std::vector<Widget*> const& children = childrenOf(container);
                // first determine the number of visible children to layout, the total width occupied by children with fixed width and number of children with width hint set to auto
                int visibleChildren = 0;
                int autoChildren = 0;
                int fixedWidth = 0;
                for (Widget* child : children)
                    if (child->visible()) {
                        ++visibleChildren;
                        if (child->widthHint().isFixed()) {
                            fixedWidth += child->width();
                            layout(container, child, child->x(), child->width(), h);
                        } else if (child->widthHint().isAuto()) {
                            ++autoChildren;
                        }
                    }
                // nothing to layout if there are no kids
                if (visibleChildren == 0)
                    return;
                // set the width and height of the percentage wanting children
                int totalWidth = clientCanvas.width();
                int availableWidth = std::max(0, totalWidth - fixedWidth);
                for (Widget* child : children)
                    if (child->widthHint().isPercentage() && child->visible()) {
                        // determine the height 
                        int w = (totalWidth - fixedWidth) * child->widthHint().pct() / 100;
                        layout(container, child, child->x(), w, h);
                        // this is important because the height might have been adjusted by the widget itself when it was set by the layout
                        availableWidth -= child->width();
                    }
                // now we know the available height for the auto widgets
                if (autoChildren != 0) {
                    int w = availableWidth / autoChildren;
                    for (Widget* child : children) {
                        if (child->widthHint().isAuto() && child->visible()) {
                            if (--autoChildren == 0)
                                w = availableWidth;
                            layout(container, child, child->x(), w, h);
                            availableWidth -= child->width();
                        }
                    }
                }
                // finally, when the sizes are right, we must reposition the elements
                int left = 0;
                for (Widget* child : children) {
                    if (child->visible()) {
                        setChildGeometry(container, child, left, 0, child->width(), child->height());
                        left += child->width();
                    }
                }
            }

            /** None of the children are overlaid.
             */
            void calculateOverlay(Container* container, bool initialOverlay) override {
                for (Widget* child : childrenOf(container))
                    setOverlayOf(child, initialOverlay);
            }

            void layout(Container* container, Widget* child, int left, int width, int maxHeight) {
                if (child->heightHint().isFixed())
                    setChildGeometry(container, child, left, 0, width, child->height());
                else if (child->heightHint().isAuto())
                    setChildGeometry(container, child, left, 0, width, maxHeight);
                else
                    setChildGeometry(container, child, left, 0, width, maxHeight * child->heightHint().pct() / 100);
            }
        }; // VerticalImpl
    } // anonymous namespace


    Layout * Layout::None = new NoneImpl();

    Layout * Layout::Maximized = new MaximizedImpl();

    Layout * Layout::HorizontalTop = new HorizontalTopImpl();

    Layout * Layout::HorizontalBottom = new HorizontalBottomImpl();

    Layout * Layout::Vertical = new VerticalImpl();

    void Layout::calculateOverlay(Container* container, bool initialOverlay) {
        Rect overlay;
        for (auto i = childrenOf(container).rbegin(), e = childrenOf(container).rend(); i != e; ++i) {
            if (!(*i)->visible())
                continue;
            Rect childRect = (*i)->rect();
            (*i)->setOverlay(initialOverlay || !Rect::Intersection(childRect, overlay).empty());
            overlay = Rect::Union(childRect, overlay);
        }
    }


    std::vector<Widget*> const& Layout::childrenOf(Container* container) {
        return container->children_;
    }

    void Layout::setChildGeometry(Container* container, Widget* child, int x, int y, int width, int height) {
        container->setChildGeometry(child, x, y, width, height);
    }

    void Layout::setOverlayOf(Widget * child, bool value) {
        child->setOverlay(value);
    }

    void Layout::relayoutWidget(Widget* child, int width, int height) {
        child->relayout(width, height);
    }


}
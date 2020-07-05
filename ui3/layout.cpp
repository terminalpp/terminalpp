#include "geometry.h"
#include "widget.h"

#include "layout.h"

namespace ui3 {

    namespace {

        class NoneLayout : public Layout {
        public:

            void layout(Widget * widget) const override {
                MARK_AS_UNUSED(widget);
                // actually do nothing
            }


        }; // NoneLayout

        class MaximizedLayout : public Layout {
        public:
            void layout(Widget * widget) const override {
                /*
                Size size = widget->contentsSize();
                for (Widget * child : children(widget)) {

                }
                MARK_AS_UNUSED(widget);
                */
            }

            void calculateOverlay(Widget * widget) const override {
                MARK_AS_UNUSED(widget);

            }

        }; // MaximizedLayout


    } // anonymous namespace

    Layout * Layout::None() {
        return new NoneLayout{};
    }

    Layout * Layout::Maximized() {
        return new MaximizedLayout{};
    }

    void Layout::calculateOverlay(Widget * widget) const {
        Rect r;
        for (Widget * child : widget->children_) {
            child->overlaid_ = ! (r & child->rect()).empty();
            r = r | child->rect();
        }
    }


} // namespace ui3
#include "geometry.h"
#include "widget.h"

#include "layout.h"

namespace ui3 {

    namespace {

        class NoLayout : public Layout {
        public:

            void layout(Widget * widget) const override {
                // actually do nothing
            }


        }; // NoLayout


    } // anonymous namespace

    Layout * const Layout::None = new NoLayout{};

    void Layout::calculateOverlay(Widget * widget) const {
        Rect r;
        for (Widget * child : widget->children_) {
            child->overlaid_ = ! (r & child->rect()).empty();
            r = r | child->rect();
        }
    }


} // namespace ui3
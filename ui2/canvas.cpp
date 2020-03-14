#include "canvas.h"
#include "widget.h"
#include "renderer.h"

namespace ui2 {

    Canvas::Canvas(Widget * widget):
        width_{widget->rect_.width()},
        height_{widget->rect_.height()},
        buffer_{widget->renderer()->buffer()},
        visibleRect_{widget->visibleRect_},
        bufferOffset_{widget->bufferOffset_} {
    }

    Canvas & Canvas::fillRect(Rect const & rect) {
        Rect r = (rect & visibleRect_) + bufferOffset_;
        for (int row = r.top(), re = r.bottom(); row < re; ++row) {
            for (int col = r.left(), ce = r.right(); col < ce; ++col) {
                applyBrush(buffer_.at(col, row), bg_);
            }
        }
        return *this;
    }


} // namespace ui2
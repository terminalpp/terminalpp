#include "../renderer.h"
#include "../widget.h"
#include "canvas.h"

namespace ui {

    Canvas::Canvas(Widget * forWidget):
        buffer_{forWidget->renderer_->buffer_},
        size_{forWidget->size()},
        visibleRect_{forWidget->visibleRect_} {
        ASSERT(IN_UI_THREAD);
    }


} // namespace ui


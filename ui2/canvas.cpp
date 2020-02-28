#include "canvas.h"
#include "widget.h"
#include "renderer.h"

namespace ui2 {

    Canvas::Canvas(Widget * widget):
        buffer_{widget->renderer()->buffer()},
        visibleRect_{widget->visibleRect_},
        bufferOffset_{widget->bufferOffset_} {
    }

} // namespace ui2
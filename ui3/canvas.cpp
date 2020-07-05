#include "renderer.h"
#include "widget.h"

#include "canvas.h"

namespace ui3 {

    Canvas::Canvas(VisibleArea const & visibleArea, Size const & size):
        visibleArea_{visibleArea},
        buffer_{visibleArea.renderer()->buffer_}, 
        size_{size} {
    }

    Canvas & Canvas::fill(Rect const & rect, Color color) {
        Rect r = (rect & visibleArea_.rect()) + visibleArea_.offset();
        if (color.opaque()) {
            for (int y = r.top(), ye = r.bottom(); y < ye; ++y) {
                for (int x = r.left(), xe = r.right(); x < xe; ++x) {
                    Cell & c = buffer_.at(x,y);
                    c.bg() = color;
                    c.codepoint() = ' ';
                    // TODO border
                }
            }
        } else {
            for (int y = r.top(), ye = r.bottom(); y < ye; ++y) {
                for (int x = r.left(), xe = r.right(); x < xe; ++x) {
                    Cell & c = buffer_.at(x,y);
                    c.fg() = color.blendOver(c.fg());
                    c.bg() = color.blendOver(c.bg());
                    c.decor() = color.blendOver(c.decor());
                    // TODO border
                }
            }
        }
        return *this;
    }


} // namespace ui
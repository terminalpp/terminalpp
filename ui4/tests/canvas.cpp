#include "helpers/tests.h"

#include "../core/canvas.h"

namespace ui {
    
    TEST(VisibleRect, CoordTransformations) {
        Canvas::VisibleRect x{Rect{Point{10,10}, Size{100, 100}}, Point{-10, -10}};
        EXPECT(x.toBuffer(Point{10,10}) == Point{0,0});
    }


} // namespace ui
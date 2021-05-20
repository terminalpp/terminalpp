#include "helpers/tests.h"

#include "../core/geometry.h"

namespace ui {

    TEST(Geometry, RectIntersection) {
        Rect a{Point{0,0}, Point{10,10}};
        Rect b{Point{5,5}, Point{7,8}};
        EXPECT((a & b) == b);
        Rect c{Point{5, 6}, Point{30,30}};
        EXPECT((a & c) == Rect{Point{5,6}, Point{10,10}});
        Rect d{Point{-5,5}, Point {5, 20}};
        EXPECT((a & d) == Rect{Point{0,5}, Point{5,10}});
        Rect e{Point{100,100}, Size{10,10}};
        EXPECT((a & e) == Rect::Empty());
    }

    TEST(Geometry, RectUnion) {
        Rect a{Point{0,0}, Point{10,10}};
        Rect b{Point{5,5}, Point{7,8}};
        EXPECT((a | b) == a);
        Rect c{Point{5, 6}, Point{30,30}};
        EXPECT((a | c) == Rect{Point{0,0}, Point{30,30}});
        Rect d{Point{-5,5}, Point {5, 20}};
        EXPECT((a | d) == Rect{Point{-5,0}, Point{10,20}});
        Rect e{Point{100,100}, Size{10,10}};
        EXPECT((a | e) == Rect{Point{0,0}, Point{110, 110}});
    }

} // namespace ui
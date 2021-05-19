#include "helpers/tests.h"

#include "../core/canvas.h"

namespace ui {
    
    TEST(VisibleRect, CoordTransformations) {
        Canvas::VisibleRect x{Rect{Point{10,10}, Size{100, 100}}, Point{-10, -10}};
        EXPECT(x.toBuffer(Point{10,10}) == Point{0,0});
        EXPECT(x.toLocal(Point{10,10}) == Point{20,20});
    }

    TEST(VisibleRect, Offset) {
        Canvas::VisibleRect x{Rect{Point{10,10}, Size{100, 100}}, Point{-10, -10}};
        Canvas::VisibleRect y{x.offsetBy(Point{20,15})};
        EXPECT(y.toBuffer(Point{30,25}) == Point{0,0});
        EXPECT(y.toBuffer(Point{10,5}) == Point{-20, -20});
        EXPECT(y.toBuffer(Point{10,10}) == Point{-20, -15});
        EXPECT(x.toBuffer(Point{0,0}) == y.toBuffer(Point{20, 15}));
        EXPECT(y.rect.topLeft() == Point{30,25});
        EXPECT(y.offset == Point{-30, -25});
    }

    TEST(VisibleRect, Clip) {
        Canvas::VisibleRect x{Rect{Point{10,10}, Size{100, 100}}, Point{-10, -10}};
        Canvas::VisibleRect y{x.clip(Rect{Point{20,20}, Size{10,10}})};
        EXPECT(y.rect == Rect{Point{0,0}, Point{10,10}});
        EXPECT(y.offset == Point{10,10});
        y = x.clip(Rect{Point{5,10}, Size{10,20}});
        EXPECT(y.rect == Rect{Point{5,0}, Point{10,20}});
        EXPECT(y.offset == Point{-5,0});
        y = x.clip(Rect{Point{100,100}, Size{20,20}});
        EXPECT(y.rect == Rect{Point{0,0}, Point{10,10}});
        EXPECT(y.offset == Point{90,90});
        y = x.clip(Rect{Point{110,110}, Size{20,20}});
        EXPECT(y.rect.empty());
        EXPECT(y.offset == Point{100,100});
    }


} // namespace ui
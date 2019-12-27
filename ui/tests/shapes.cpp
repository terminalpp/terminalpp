#include "helpers/tests.h"

#include "../shapes.h"

namespace ui {

    // Point

    TEST(ui_shapes, point) {
        Point p;
        EXPECT(p.x == 0);
        EXPECT(p.y == 0);
        p.x = 10;
        EXPECT(p.x == 10);
        p.y = 11;
        EXPECT(p.y == 11);
        p.set(1, 2);
        EXPECT(p.x == 1);
        EXPECT(p.y == 2);
        Point p2(10,11);
        EXPECT(p2.x == 10);
        EXPECT(p2.y == 11);
    }

    TEST(ui_shapes, point_comparison) {
        Point p(1,2);
        Point p2(1, 2);
        EXPECT(p == p2);
        EXPECT(! (p != p2));
        p2 = Point(2, 1);
        EXPECT(p != p2);
        EXPECT(! (p == p2)); 
    }

    TEST(ui_shapes, point_origin) {
        Point p(1, 0);
        EXPECT(! p.isOrigin());
        p.x = 0;
        EXPECT(p.isOrigin());
    }

    TEST(ui_shapes, point_add) {
        EXPECT(Point{10, 20} + Point{2, 1} == Point{12, 21});
        EXPECT(Point{10, 20} + 3 == Point{13,23});
        EXPECT((Point{10, 20} += Point{10,20}) == Point{20, 40});
        EXPECT((Point{10, 20} += 3) == Point{13, 23});
    }

    TEST(ui_shapes, point_sub) {
        EXPECT(Point{10, 20} - Point{2, 1} == Point{8, 19});
        EXPECT(Point{10, 20} - 3 == Point{7,17});
        EXPECT((Point{10, 20} -= Point{10,20}) == Point{0, 0});
        EXPECT((Point{10, 20} -= 3) == Point{7, 17});
    }

    // Rect

    TEST(ui_shapes, rect_create) {
        EXPECT(Rect{}.empty());
    }

    TEST(ui_shapes, rect_intersection) {
        EXPECT_EQ(Rect::Intersection(Rect::FromTopLeftWH(0, 0, 10, 10), Rect::FromTopLeftWH(5, 3, 20, 20)), Rect::FromCorners(Point{5, 3}, Point{10, 10}));
        EXPECT(Rect::Intersection(Rect::FromTopLeftWH(0,0,5, 5), Rect::FromTopLeftWH(10,10,10,10)).empty());
        // intersection with empty is empty
        EXPECT(Rect::Intersection(Rect::FromTopLeftWH(10,10, 10, 10), Rect::Empty()).empty());
    }

    TEST(ui_shapes, rect_union) {
        EXPECT_EQ(Rect::Union(Rect::FromTopLeftWH(10,10,10,10), Rect::Empty()), Rect::FromTopLeftWH(10,10,10,10));
    }

} // namespace ui
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

    TEST(Geometry, ColorConstructors) {
        Color c = Color::RGB(1,2,3);
        EXPECT(c.r == 1);
        EXPECT(c.g == 2);
        EXPECT(c.b == 3);
        EXPECT(c.a == 255);
        c = Color::HTML("#c0c0ff");
        EXPECT(c.r == 0xc0);
        EXPECT(c.g == 0xc0);
        EXPECT(c.b == 0xff);
        EXPECT(c.a == 255);
        c = Color::HTML("aabbcc");
        EXPECT(c.r == 0xaa);
        EXPECT(c.g == 0xbb);
        EXPECT(c.b == 0xcc);
        EXPECT(c.a == 255);
        c = Color::RGBA(1,2,3,4);
        EXPECT(c.r == 1);
        EXPECT(c.g == 2);
        EXPECT(c.b == 3);
        EXPECT(c.a == 4);
        c = Color::HTML("#c0c0ff80");
        EXPECT(c.r == 0xc0);
        EXPECT(c.g == 0xc0);
        EXPECT(c.b == 0xff);
        EXPECT(c.a == 0x80);
        c = Color::HTML("aabbcc40");
        EXPECT(c.r == 0xaa);
        EXPECT(c.g == 0xbb);
        EXPECT(c.b == 0xcc);
        EXPECT(c.a == 0x40);
        EXPECT_THROWS(IOError, 
            Color::HTML("foobar");
        )
        EXPECT_THROWS(IOError, 
            Color::HTML("#aa");
        )
        EXPECT_THROWS(IOError, 
            Color::HTML("#aabbccddee");
        )
        EXPECT_THROWS(IOError, 
            Color::HTML("aabb");
        )
        EXPECT_THROWS(IOError, 
            Color::HTML("aabbccddee");
        )
    }

    TEST(Geometry, ColorComparison) {
        Color c = Color::HTML("#aabbcc");
        Color d = Color::HTML("#ccbbaa");
        EXPECT(c == c);
        EXPECT(c != d);
    }

    TEST(Geometry, ColorConverters) {
        Color c = Color::HTML("#aabbccdd");
        EXPECT_EQ(c.toRGB(), 0xaabbcc);
        EXPECT_EQ(c.toRGBA(), 0xaabbccdd);
    }

    TEST(Geometry, ColorOverlay) {
        Color c = Color::HTML("#102030");
        EXPECT_EQ(c.overlayWith(Color::HTML("#aabbcc")), Color::HTML("#aabbcc"));
        EXPECT_EQ(c.overlayWith(Color::HTML("#aabbcc00")), c);
        EXPECT_EQ(c.overlayWith(Color::HTML("#aabbcc80")), Color::HTML("#5d6e7e"));
        c = Color::HTML("#ffffff");
        EXPECT_EQ(c.overlayWith(Color::HTML("#0000cc40")), Color::HTML("#bfbff3"));
    }


} // namespace ui
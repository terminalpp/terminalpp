#include "helpers/tests.h"

#include "ui/canvas.h"

using namespace ui;

class Canvas::TestAdapter {
public:

    static Point VisibleAreaOffset(Canvas & c) {
        return c.visibleArea_.offset();
    }

}; // TestAdapter

TEST(canvas, create) {
    Canvas::Buffer b{Size{100,100}};
    Canvas c{b};
    EXPECT_EQ(c.visibleRect(), Rect{Size{100,100}});
    EXPECT_EQ(c.size(), Size{100,100});
    EXPECT_EQ(Canvas::TestAdapter::VisibleAreaOffset(c), Point{0,0});
}

TEST(canvas, resize_up) {
    Canvas::Buffer b{Size{100,100}};
    Canvas c{b};
    Canvas cc{c.resize(Size{200,300})};
    EXPECT_EQ(cc.visibleRect(), Rect{Size{100,100}});
    EXPECT_EQ(cc.size(), Size{200,300});
}

TEST(canvas, resize_down) {
    Canvas::Buffer b{Size{100,100}};
    Canvas c{b};
    Canvas cc{c.resize(Size{50,40})};
    EXPECT_EQ(cc.visibleRect(), Rect{Size{50,40}});
    EXPECT_EQ(cc.size(), Size{50,40});
}

TEST(canvas, clip_init) {
    Canvas::Buffer b{Size{100,100}};
    Canvas c{b};
    Canvas cc{c.clip(Rect{Size{50,40}})};
    EXPECT_EQ(cc.visibleRect(), Rect{Size{50,40}});
    EXPECT_EQ(cc.size(), Size{50,40});
}

TEST(canvas,clip_inside) {
    Canvas::Buffer b{Size{100,100}};
    Canvas c{b};
    Canvas cc{c.clip(Rect{Point{10,10}, Size{50,40}})};
    EXPECT_EQ(cc.visibleRect(), Rect{Size{50,40}});
    EXPECT_EQ(cc.size(), Size{50,40});
    EXPECT_EQ(Canvas::TestAdapter::VisibleAreaOffset(cc), Point{10,10});
}

TEST(canvas,clip_overlap) {
    Canvas::Buffer b{Size{100,100}};
    Canvas c{b};
    Canvas cc{c.resize(Size{200,200}).clip(Rect{Point{50,60}, Size{100,100}})};
    EXPECT_EQ(cc.visibleRect(), Rect{Size{50,40}});
    EXPECT_EQ(Canvas::TestAdapter::VisibleAreaOffset(cc), Point{50,60});
}

TEST(canvas,clip_overlap_oversize) {
    Canvas::Buffer b{Size{100,100}};
    Canvas c{b};
    Canvas cc{c.clip(Rect{Point{50,60}, Size{100,100}})};
    EXPECT_EQ(cc.visibleRect(), Rect{Size{50,40}});
    EXPECT_EQ(Canvas::TestAdapter::VisibleAreaOffset(cc), Point{50,60});
}

TEST(canvas,clip_overlap_negative) {
    Canvas::Buffer b{Size{100,100}};
    Canvas c{b};
    Canvas cc{c.clip(Rect{Point{-50,-60}, Size{100,100}})};
    EXPECT_EQ(cc.visibleRect(), Rect{Point{50, 60},Size{50,40}});
    EXPECT_EQ(Canvas::TestAdapter::VisibleAreaOffset(cc), Point{-50,-60});
}

TEST(canvas,clip_outside) {
    Canvas::Buffer b{Size{100,100}};
    Canvas c{b};
    Canvas cc{c.clip(Rect{Point{1000,1000}, Size{100,100}})};
    EXPECT_EQ(cc.visibleRect().empty(), true);
}




// cursor

TEST(canvas, set_cursor) {
    Canvas::Buffer b{Size{100,100}};
    EXPECT_EQ(b.cursorPosition(), Canvas::Buffer::NoCursorPosition);
    Canvas c{b};
    Canvas::Cursor cursor;
    c.setCursor(cursor, Point{1,2});
    EXPECT_EQ(b.cursorPosition(), Point{1,2});
}

TEST(canvas, set_cursor_outside) {
    Canvas::Buffer b{Size{100,100}};
    Canvas c{b};
    Canvas::Cursor cursor;
    c.setCursor(cursor, Point{50,200});
    EXPECT_EQ(b.cursorPosition(), Canvas::Buffer::NoCursorPosition);
}
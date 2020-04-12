#include "helpers/tests.h"

#include "../canvas.h"
#include "../root_window.h"

namespace ui {

    // Canvas::Cell

    TEST(ui_canvas, cell_font) {
        Cell c;
        c.setCodepoint(0);
        EXPECT_EQ(c.codepoint(), (char32_t) 0);
        c.setFont(Font().setBold().setSize(2));
        EXPECT_EQ(c.codepoint(), (char32_t) 0);
        EXPECT(c.font().bold());
        EXPECT_EQ(c.font().size(), 2);
    }

    // Canvas

    class TestCanvasAccessor {
    public:

        Canvas::VisibleRect createVisibleRect(Rect const & rect, Point const & bufferOffset, RootWindow & root) {
            return Canvas::VisibleRect{rect, bufferOffset, & root};
        }

        Rect & getVisibleRect(Canvas & canvas) {
            return canvas.visibleRect_.rect_;
        }

        Point & getVisibleRectOffset(Canvas & canvas) {
            return canvas.visibleRect_.bufferOffset_;
        }

        bool & getVisibleRectValid(Canvas & canvas) {
            return canvas.visibleRect_.valid_;
        }

        bool getVisibleRectEmpty(Canvas & canvas) {
            return canvas.visibleRect_.empty();
        }

        RootWindow * & getVisibleRectRootWindow(Canvas & canvas) {
            return canvas.visibleRect_.rootWindow_;
        }


    }; // ui::TestCanvasAccessor 

    TEST(ui_canvas, canvas_for_widget, TestCanvasAccessor) {
        RootWindow rw{100, 200};
        Canvas c{&rw};
        EXPECT_EQ(c.width(), 100);
        EXPECT_EQ(c.height(), 200);
        EXPECT(! getVisibleRectEmpty(c));
    }

} // namespace ui
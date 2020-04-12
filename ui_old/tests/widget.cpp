#include "helpers/tests.h"

#include "../widget.h"

namespace ui {

    class TestWidget : public Widget {
    public:
        TestWidget(int width, int height):
            Widget(width, height) {
        }

    protected:
        void paint(Canvas & canvas) {
            MARK_AS_UNUSED(canvas);
        }
    }; 

    TEST(ui_widget, constructor_sets_size) {
        TestWidget w{80,25};
        EXPECT_EQ(w.width(), 80);
        EXPECT_EQ(w.height(), 25);
    }

} // namespace ui

#include "helpers/tests.h"

#include "../container.h"

namespace ui {

    class TestContainer : public Container {
    public:
        TestContainer(int width, int height):
            Container(width, height) {
        }
    }; 

    TEST(ui_container, constructor_sets_size) {
        TestContainer c{80,25};
        EXPECT_EQ(c.width(), 80);
        EXPECT_EQ(c.height(), 25);
    }

} // namespace ui

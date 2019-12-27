#include "helpers/tests.h"

#include "../root_window.h"

namespace ui {

    TEST(ui_root_window, constructor_sets_size) {
        RootWindow rw{80,25};
        EXPECT_EQ(rw.width(), 80);
        EXPECT_EQ(rw.height(), 25);
    }

} // namespace ui

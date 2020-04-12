#include "helpers/tests.h"

#include "../color.h"

namespace ui {

    // Color

    TEST(ui_color, color_create) {
        Color c{32,64,128};
        EXPECT_EQ(c.r, 32);
        EXPECT_EQ(c.g, 64);
        EXPECT_EQ(c.b, 128);
        EXPECT_EQ(c.a, 255);
        c = Color::FromHTML("#102030ff");
        EXPECT_EQ(c.r, 0x10);
        EXPECT_EQ(c.g, 0x20);
        EXPECT_EQ(c.b, 0x30);
        EXPECT_EQ(c.a, 255);
        c = Color::FromHTML("#10203080");
        EXPECT_EQ(c.r, 0x10);
        EXPECT_EQ(c.g, 0x20);
        EXPECT_EQ(c.b, 0x30);
        EXPECT_EQ(c.a, 128);
    }

} // namespace ui
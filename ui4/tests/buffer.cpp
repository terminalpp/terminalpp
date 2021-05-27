#include "helpers/tests.h"

#include "../core/buffer.h"

namespace ui {

    TEST(BufferCell, Codepoint) {
        Buffer::Cell cell{};
        EXPECT_EQ(cell.codepoint(), ' ');
        cell.setCodepoint('X');
        EXPECT_EQ(cell.codepoint(), 'X');
    }

    TEST(BufferCell, Defaults) {
        Buffer::Cell cell{};
        EXPECT(cell.border().empty());
        EXPECT_EQ(cell.font().size(), 1);
        EXPECT_EQ(cell.specialObject(), nullptr);
    }

}
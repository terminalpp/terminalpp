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

    class TestSpecialObject : public Buffer::CellSpecialObject {
    public:

        TestSpecialObject() {
            ++Instances;
        }

        ~TestSpecialObject() override {
            --Instances;
        }

        static size_t Instances;
    }; 

    size_t TestSpecialObject::Instances = 0;

    TEST(BufferCell, SpecialObjects) {
        Buffer::Cell cell{};
        EXPECT_EQ(TestSpecialObject::Instances, 0);
        TestSpecialObject * t = new TestSpecialObject{};
        EXPECT_EQ(TestSpecialObject::Instances, 1);
        cell.setSpecialObject(t);
        EXPECT_EQ(cell.specialObject(), t);
        EXPECT_EQ(TestSpecialObject::Instances, 1);
        {
            Buffer::Cell c2{};
            c2.setSpecialObject(cell.specialObject());
            EXPECT_EQ(c2.specialObject(), t);
            EXPECT_EQ(TestSpecialObject::Instances, 1);
        }
        EXPECT_EQ(TestSpecialObject::Instances, 1);
        cell.setSpecialObject(nullptr);
        EXPECT_EQ(TestSpecialObject::Instances, 0);
    }

}
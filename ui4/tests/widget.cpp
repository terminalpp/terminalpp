#include "helpers/tests.h"
#include "../widget.h"

namespace ui {

    class TestWidget : public Widget {
    protected:
        void paint(Canvas & canvas) override {

        }
    };

    TEST(ui_widget, default_constructor) {
        std::unique_ptr<Widget> w{new TestWidget{}};
        EXPECT(w->parent() == nullptr);
        EXPECT(w->previousSibling() == nullptr);
        EXPECT(w->nextSibling() == nullptr);
        EXPECT(w->frontChild() == nullptr);
        EXPECT(w->backChild() == nullptr);
    }

    TEST(ui_widget, append_child) {
        std::unique_ptr<Widget> w{new TestWidget{}};
        Widget * c1 = new TestWidget{};
        w->appendChild(c1);
        EXPECT(w->frontChild() == c1);        
        EXPECT(w->backChild() == c1);        
        Widget * c2 = new TestWidget{};
        w->appendChild(c2);
        // note front child is the newly appended one
        EXPECT(w->frontChild() == c2);        
        EXPECT(w->backChild() == c1);        
    }


}
#include "helpers/tests.h"
#include "../widget.h"

namespace ui {

    TEST(ui_widget, default_constructor) {
        std::unique_ptr<Widget> w{new Widget{}};
        EXPECT(w->parent() == nullptr);
        EXPECT(w->previousSibling() == nullptr);
        EXPECT(w->nextSibling() == nullptr);
        EXPECT(w->frontChild() == nullptr);
        EXPECT(w->backChild() == nullptr);
    }

    TEST(ui_widget, append_child) {
        std::unique_ptr<Widget> w{new Widget{}};
        Widget * c1 = new Widget{};
        w->appendChild(c1);
        EXPECT(w->frontChild() == c1);        
        EXPECT(w->backChild() == c1);        
        Widget * c2 = new Widget{};
        w->appendChild(c2);
        // note front child is the newly appended one
        EXPECT(w->frontChild() == c2);        
        EXPECT(w->backChild() == c1);        
    }

    TEST(ui_widget, events) {
        // we start with empty events queue
        EXPECT(Widget::GlobalEventDummy_->pendingEvents_ == 0);
        EXPECT(Widget::ProcessEvent() == false);
        std::string sideeffect;
        Widget::Schedule([&sideeffect](){
            sideeffect = "first";
        });
        EXPECT(sideeffect.empty()); // the code did not execute
        EXPECT(Widget::GlobalEventDummy_->pendingEvents_ == 1);
        EXPECT(Widget::ProcessEvent() == true);
        EXPECT(Widget::GlobalEventDummy_->pendingEvents_ == 0);
        EXPECT(sideeffect == "first");
        EXPECT(Widget::ProcessEvent() == false);
        {
            std::unique_ptr<Widget> w{new Widget()};
            w->schedule([&sideeffect](){
                sideeffect = "second";
            });
            w->schedule([&sideeffect](){
                sideeffect = "third";
            });
            Widget::Schedule([&sideeffect](){
                sideeffect = "fourth";
            });
            w->schedule([&sideeffect](){
                sideeffect = "fifth";
            });
            EXPECT(Widget::GlobalEventDummy_->pendingEvents_ == 1);
            EXPECT(w->pendingEvents_ == 3);
            EXPECT(Widget::ProcessEvent() == true);
            EXPECT(Widget::GlobalEventDummy_->pendingEvents_ == 1);
            EXPECT(w->pendingEvents_ == 2);
            EXPECT(sideeffect == "second");
            // now this should delete the events attached to w, but keep the unattached event
        }
        EXPECT(Widget::GlobalEventDummy_->pendingEvents_ == 1);
        EXPECT(Widget::ProcessEvent() == true);
        EXPECT(Widget::GlobalEventDummy_->pendingEvents_ == 0);
        EXPECT(sideeffect == "fourth");
        EXPECT(Widget::ProcessEvent() == false);
    }


}
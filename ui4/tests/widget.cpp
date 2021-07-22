#include "helpers/tests.h"
#include "../widget.h"

namespace ui {

    TEST(ui_widget, default_constructor) {
        std::unique_ptr<Widget> w = std::make_unique<Widget>();
        EXPECT(w->parent() == nullptr);
        EXPECT(w->previousSibling() == nullptr);
        EXPECT(w->nextSibling() == nullptr);
        EXPECT(w->frontChild() == nullptr);
        EXPECT(w->backChild() == nullptr);
    }

    TEST(ui_widget, append_child) {
        std::unique_ptr<Widget> w = std::make_unique<Widget>();
        Widget * c1 = new Widget{};
        w->appendChild(c1);
        EXPECT(w->frontChild() == c1);        
        EXPECT(w->backChild() == c1);        
        Widget * c2 = new Widget{};
        w->appendChild(c2);
        // note front child is the newly appended one
        EXPECT(w->frontChild() == c2);        
        EXPECT(w->backChild() == c1);        
        // appending same child again moves it to the front
        w->appendChild(c1);
        EXPECT(w->frontChild() == c1);        
        EXPECT(w->backChild() == c2);        
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

    TEST(ui_widget, CommonParent) {
        std::unique_ptr<Widget> a = std::make_unique<Widget>();
        std::unique_ptr<Widget> b = std::make_unique<Widget>();
        EXPECT(Widget::CommonParent(a.get(), b.get()) == nullptr);
        EXPECT(Widget::CommonParent(a.get(), nullptr) == a.get());
        EXPECT(Widget::CommonParent(nullptr, b.get()) == b.get());
        Widget * c1 = new Widget{};
        a->appendChild(c1);
        Widget * c2 = new Widget{};
        c1->appendChild(c2);
        Widget * c3 = new Widget{};
        a->appendChild(c3);
        EXPECT(Widget::CommonParent(c1, c2) == c1);
        EXPECT(Widget::CommonParent(c2, c1) == c1);
        EXPECT(Widget::CommonParent(c1, c3) == a.get());
    }

    TEST(ui_widget, Siblings) {
        std::unique_ptr<Widget> a{ new Widget{} };
        Widget * b = a->appendChild(new Widget{});
        EXPECT(b->previousSibling() == nullptr);
        EXPECT(b->nextSibling() == nullptr);
        Widget * c = a->appendChild(new Widget{});
        EXPECT(b->previousSibling() == nullptr);
        EXPECT(b->nextSibling() == c);
        EXPECT(c->previousSibling() == b);
        EXPECT(c->nextSibling() == nullptr);
        Widget * d = a->appendChild(new Widget{});
        EXPECT(c->previousSibling() == b);
        EXPECT(c->nextSibling() == d);
    }

    TEST(ui_widget, ChildIterator) {
        std::unique_ptr<Widget> a{ new Widget{} };
        Widget * b = a->appendChild(new Widget{});
        Widget * c = a->appendChild(new Widget{});
        Widget * d = a->appendChild(new Widget{});
        auto i = a->begin();
        EXPECT(*i == b);
        ++i;
        EXPECT(*i == c);
        ++i;
        EXPECT(*i == d);
        ++i;
        EXPECT(i == a->end());
    }

    TEST(ui_widget, ChildOrder) {
        std::unique_ptr<Widget> a = std::make_unique<Widget>();
        EXPECT(a->frontChild() == nullptr);
        EXPECT(a->backChild() == nullptr);
        Widget * b = a->appendChild(new Widget{});
        EXPECT(a->frontChild() == b);
        EXPECT(a->backChild() == b);
        Widget * c = a->appendChild(new Widget{});
        EXPECT(a->frontChild() == c);
        EXPECT(a->backChild() == b);
        Widget * d = a->appendChild(new Widget{});
        EXPECT(a->frontChild() == d);
        EXPECT(a->backChild() == b);
    }

    TEST(ui_widget, ChildMoves) {
        std::unique_ptr<Widget> a = std::make_unique<Widget>();
        Widget * b = a->appendChild(new Widget{});
        Widget * c = a->appendChild(new Widget{});
        Widget * d = a->appendChild(new Widget{});
        d->moveToFront();
        {
            EXPECT(b->previousSibling() == nullptr);
            EXPECT(b->nextSibling() == c);
            EXPECT(c->previousSibling() == b);
            EXPECT(c->nextSibling() == d);
            EXPECT(d->previousSibling() == c);
            EXPECT(d->nextSibling() == nullptr);
        } 
        b->moveToFront();
        {
            EXPECT(c->previousSibling() == nullptr);
            EXPECT(c->nextSibling() == d);
            EXPECT(d->previousSibling() == c);
            EXPECT(d->nextSibling() == b);
            EXPECT(b->previousSibling() == d);
            EXPECT(b->nextSibling() == nullptr);
        }
        c->moveForward();
        {
            EXPECT(d->previousSibling() == nullptr);
            EXPECT(d->nextSibling() == c);
            EXPECT(c->previousSibling() == d);
            EXPECT(c->nextSibling() == b);
            EXPECT(b->previousSibling() == c);
            EXPECT(b->nextSibling() == nullptr);
        }
        c->moveForward();
        {
            EXPECT(d->previousSibling() == nullptr);
            EXPECT(d->nextSibling() == b);
            EXPECT(b->previousSibling() == d);
            EXPECT(b->nextSibling() == c);
            EXPECT(c->previousSibling() == b);
            EXPECT(c->nextSibling() == nullptr);
        }
        c->moveBackward();
        {
            EXPECT(d->previousSibling() == nullptr);
            EXPECT(d->nextSibling() == c);
            EXPECT(c->previousSibling() == d);
            EXPECT(c->nextSibling() == b);
            EXPECT(b->previousSibling() == c);
            EXPECT(b->nextSibling() == nullptr);
        }
        c->moveBackward();
        {
            auto i = a->begin();
            EXPECT(*i++ == c);
            EXPECT(*i++ == d);
            EXPECT(*i++ == b);
            EXPECT(i == a->end());
        }
        b->moveToBack();
        {
            auto i = a->begin();
            EXPECT(*i++ == b);
            EXPECT(*i++ == c);
            EXPECT(*i++ == d);
            EXPECT(i == a->end());
        } 
    }


}
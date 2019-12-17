#include "../tests.h"
#include "../events.h"

namespace helpers {

    namespace {

        class EventSender {
        public:
            size_t triggers = 0;
            EventSender * sender;
            void voidHandler(Event<void, EventSender>::Payload & e) {
                ++triggers;
                sender = e.sender();
            }

            void intHandler(Event<int, EventSender>::Payload & e) {
                triggers += *e;
                sender = e.sender();
            }
        };

        size_t & EventTriggers() {
            static size_t count = 0;
            return count;
        }

        void VoidHandler(Event<void, EventSender>::Payload &) {
            ++EventTriggers();
        }

        void IntHandler(Event<int, EventSender>::Payload & e) {
            EventTriggers() += *e;
        }

    } // anonymous namespace

    TEST(helpers_events, function_trigger_void) {
        Event<void, EventSender> e;
        EventTriggers() = 0;

        EXPECT( ! e.attached());
        EXPECT_EQ(EventTriggers(), 0);
        e(nullptr);
        EXPECT_EQ(EventTriggers(), 0);
        e = VoidHandler;
        EXPECT(e.attached());
        e(nullptr);
        EXPECT_EQ(EventTriggers(), 1);
        e.clear();
        EXPECT( ! e.attached());
        EXPECT_EQ(EventTriggers(), 1);
        e.setHandler(VoidHandler);
        EXPECT(e.attached());
        e(nullptr);
        EXPECT_EQ(EventTriggers(), 2);
        e.clear();
        EXPECT( ! e.attached());
        EXPECT_EQ(EventTriggers(), 2);
    }

    TEST(helpers_events, stdfunction_trigger_void) {
        size_t triggers = 0;
        Event<void, EventSender> e;
        EXPECT_EQ(triggers, 0);
        e = [&triggers](Event<void, EventSender>::Payload &) {
            ++triggers;
        };
        EXPECT(e.attached());
        e(nullptr);
        EXPECT_EQ(triggers, 1);
        e.clear();
        EXPECT( ! e.attached());
        e(nullptr);
        EXPECT_EQ(triggers, 1);
        e.setHandler(
            [&triggers](Event<void, EventSender>::Payload &) {
                ++triggers;
            }            
        );
        EXPECT(e.attached());
        e(nullptr);
        EXPECT_EQ(triggers, 2);
    }

    TEST(helpers_events, method_trigger_void) {
        Event<void, EventSender> e;
        EventSender sender;
        EXPECT_EQ(sender.triggers, 0);
        e.setHandler(&EventSender::voidHandler, &sender);
        EXPECT(e.attached());
        e(nullptr);
        EXPECT_EQ(sender.triggers, 1);
        e.clear();
        EXPECT(! e.attached());
        e(nullptr);
        EXPECT_EQ(sender.triggers, 1);
        e.setHandler(&EventSender::voidHandler, &sender);
        EXPECT(e.attached());
        e(nullptr);
        EXPECT_EQ(sender.triggers, 2);
    }

    TEST(helpers_events, function_trigger_payload) {
        Event<int, EventSender> e;
        EventTriggers() = 0;

        EXPECT( ! e.attached());
        EXPECT_EQ(EventTriggers(), 0);
        e(nullptr, 10);
        EXPECT_EQ(EventTriggers(), 0);
        e = IntHandler;
        EXPECT(e.attached());
        e(nullptr, 20);
        EXPECT_EQ(EventTriggers(), 20);
        e.clear();
        EXPECT( ! e.attached());
        EXPECT_EQ(EventTriggers(), 20);
        e.setHandler(IntHandler);
        EXPECT(e.attached());
        e(nullptr, 30);
        EXPECT_EQ(EventTriggers(), 50);
        e.clear();
        EXPECT( ! e.attached());
        EXPECT_EQ(EventTriggers(), 50);
    }

    TEST(helpers_events, stdfunction_trigger_payload) {
        size_t triggers = 0;
        Event<int, EventSender> e;
        EXPECT_EQ(triggers, 0);
        e = [&triggers](Event<int, EventSender>::Payload & e) {
            triggers += *e;
        };
        EXPECT(e.attached());
        e(nullptr, 10);
        EXPECT_EQ(triggers, 10);
        e.clear();
        EXPECT( ! e.attached());
        e(nullptr, 20);
        EXPECT_EQ(triggers, 10);
        e.setHandler(
            [&triggers](Event<int, EventSender>::Payload & e) {
                triggers += *e;
            }            
        );
        EXPECT(e.attached());
        e(nullptr, 30);
        EXPECT_EQ(triggers, 40);
    }

    TEST(helpers_events, method_trigger_payload) {
        Event<int, EventSender> e;
        EventSender sender;
        EXPECT_EQ(sender.triggers, 0);
        e.setHandler(&EventSender::intHandler, &sender);
        EXPECT(e.attached());
        e(nullptr, 10);
        EXPECT_EQ(sender.triggers, 10);
        e.clear();
        EXPECT(! e.attached());
        e(nullptr, 20);
        EXPECT_EQ(sender.triggers, 10);
        e.setHandler(&EventSender::intHandler, &sender);
        EXPECT(e.attached());
        e(nullptr, 50);
        EXPECT_EQ(sender.triggers, 60);
    }

    TEST(helpers_events, void_event_sender) {
        Event<void, EventSender> e;
        EventSender sender;
        e.setHandler(&EventSender::voidHandler, &sender);
        e(nullptr);
        EXPECT_NULL(sender.sender);
        e(&sender);
        EXPECT_EQ(sender.sender, &sender);
    }

    TEST(helpers_events, int_event_sender) {
        Event<int, EventSender> e;
        EventSender sender;
        e.setHandler(&EventSender::intHandler, &sender);
        e(nullptr, 10);
        EXPECT_NULL(sender.sender);
        EXPECT_EQ(sender.triggers, 10);
        e(&sender, 100);
        EXPECT_EQ(sender.sender, &sender);
        EXPECT_EQ(sender.triggers, 110);
    }

} // namespace helpers
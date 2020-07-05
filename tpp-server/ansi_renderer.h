#pragma once

#include "helpers/ansi_sequences.h"

#include "tpp-lib/terminal_client.h"

#include "ui3/renderer.h"


namespace ui3 {

    /** Renders the UI inside an ANSI escape sequences terminal. 
     */
    class AnsiRenderer : public Renderer, public tpp::TerminalClient {
    public:

        AnsiRenderer(tpp::PTYSlave * pty);

        ~AnsiRenderer() override;

    protected:

        void eventNotify() override {
            pushEvent(Event::User());
        }

        void render(Buffer const & buffer, Rect const & rect) override;

        void resized(int cols, int rows) override {
            MARK_AS_UNUSED(cols);
            MARK_AS_UNUSED(rows);
            // TODO this must be event
            // does not fire actually
            // Renderer::resize(Size{cols, rows});
        }

        size_t received(char const * buffer, char const * bufferEnd) override {
            MARK_AS_UNUSED(buffer);
            MARK_AS_UNUSED(bufferEnd);
            return 0;
        }

        void receivedSequence(tpp::Sequence::Kind, char const * buffer, char const * bufferEnd) {
            MARK_AS_UNUSED(buffer);
            MARK_AS_UNUSED(bufferEnd);
            NOT_IMPLEMENTED;
        }

    private:

    /** \name UI Event Loop
     */

    //@{
    
    public:
        void mainLoop() {
            while (true) {
                Event e{popEvent()};
                switch (e.kind) {
                    case Event::Kind::Terminate:
                        return;
                    case Event::Kind::User:
                        processEvent();
                        break;
                    default:
                        // TODO unknown event
                        break;
                }
            }
        }

    private:
        struct Event {
            enum class Kind {
                Terminate,
                User,
                Resize,
            }; // AnsiRenderer::Event::Kind

            Kind kind;

            static Event User() {
                return Event{Kind::User};
            }
        }; // AnsiRenderer::Event

        void pushEvent(Event const & e) {
            std::lock_guard<std::mutex> g{eventQueueGuard_};
            eventQueue_.push_back(e);
            eventQueueReady_.notify_one();
        }

        Event popEvent() {
            std::unique_lock<std::mutex> g{eventQueueGuard_};
            while (eventQueue_.empty())
                eventQueueReady_.wait(g);
            Event result = eventQueue_.front();
            eventQueue_.pop_front();
            return result;
        }

        std::deque<Event> eventQueue_;
        std::mutex eventQueueGuard_;
        std::condition_variable eventQueueReady_;
    //@}

    }; // ui::AnsiRenderer

} // namespace ui3
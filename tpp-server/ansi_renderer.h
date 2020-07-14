#pragma once

#include "helpers/ansi_sequences.h"
#include "helpers/fsm.h"

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

        void resized(ResizeEvent::Payload & e) override {
            pushEvent(Event::Resize(e->first, e->second));
        }

        size_t received(char const * buffer, char const * bufferEnd) override;
        
        void receivedSequence(tpp::Sequence::Kind, char const * buffer, char const * bufferEnd) override;

    private:

        static MatchingFSM<Key, char> VtKeys_;

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
                    case Event::Kind::Resize:
                        Renderer::resize(Size{e.payload.size.first, e.payload.size.second});
                        break;
                    case Event::Kind::KeyChar:
                        keyChar(e.payload.c);
                        break;
                    case Event::Kind::KeyDown:
                        keyDown(e.payload.k);
                        break;
                    case Event::Kind::KeyUp:
                        keyUp(e.payload.k);
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
                KeyChar,
                KeyDown,
                KeyUp
            }; // AnsiRenderer::Event::Kind

            Kind kind;

            union U {
                int i;
                Char c;
                Key k;
                std::pair<int, int> size;

                U(int i):
                    i{i} {
                }

                U(Char c):
                    c{c} {
                }

                U(Key k):
                    k{k} {
                }

                U(std::pair<int, int> const & size):
                    size{size} {
                }
            } payload;

            static Event User() {
                return Event{Kind::User, 0};
            }

            static Event Resize(int cols, int rows) {
                return Event{Kind::Resize, std::make_pair(cols, rows)};
            }

            static Event KeyChar(Char c) {
                return Event{Kind::KeyChar, c};
            }

            static Event KeyDown(Key k) {
                return Event{Kind::KeyDown, k};
            }

            static Event KeyUp(Key k) {
                return Event{Kind::KeyUp, k};
            }

        private:
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
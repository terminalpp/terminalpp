#pragma once

#include <algorithm>
#include <thread>
#include <mutex>

#include "helpers/time.h"

#include "ui/canvas.h"

#include "ui/renderer.h"
#include "ui/event_queue.h"

#include "config.h"

#include "application.h"
#include "font.h"

namespace tpp {

    using namespace ui;

    class Window : public Renderer {
    public:
        /** Determines the icon of the renderer's window where appropriate. 
         
            Instead of specifying the actual icon, which is left to the actual renderers being used, the icon specifies the meaning of the icon.
         */
        enum class Icon {
            Default,
            Notification,
        }; // Window::Icon


        std::string const & title() const {
            return title_;
        }

        virtual void setTitle(std::string const & value) {
            if (value != title_)
                title_ = value;
        }

        Icon icon() const {
            return icon_;
        }

        virtual void setIcon(Icon value) {
            if (value != icon_)
                icon_ = value;
        }

        Size sizePx() const {
            return sizePx_;
        }

        double zoom() const {
            return zoom_;
        }

        virtual void setZoom(double value) {
            if (zoom_ != value)
                zoom_ = value;
        }

        bool fullscreen() const {
            return fullscreen_;
        }

        virtual void setFullscreen(bool value = true) {
            if (fullscreen_ != value)
                fullscreen_ = value;
        }

        virtual void show(bool value = true) = 0;

        /** Determines the background color of the window. 
         
            TODO this should take the default background color from the terminal that is visible, or something like that. 
         */
        Color backgroundColor() const {
            return Color::Black;
        }

    /** \name Window Closing. 

        To request a window to close, the requestClose() method should be called, which triggers the onClose event. Unless deactivated in the handler, the close() method will be called immediately after the event is serviced. The close() method then actually closes the window. 

        This gives the window the ability to implement conditional closings such as close of a window with multiple terminals can be rejected. 

        The close() method of the window should never be called by the widgets. Rather the state should be updated in such way that the onClose handler will not deactivate the event and then requestClose() should be called. 

        TODO maybe this should move to renderer proper? See if this will be needed for ansi renderer as well. 
     */
    //@{
    public:
        using CloseEvent = ui::Event<void, Window>;

        /** Requests the window to be closed,.
            
            Generates the `onClose` event and unless the event is deactivated, calls the close() method afterwards. Can be called either programatically, or when user requests the closure of the window. 
         */
        void requestClose() {
            CloseEvent::Payload p{};
            onClose(p, this);
            if (p.active())
                close();
        }

        /** Triggered when closure of the widow is requested. 
         */
        CloseEvent onClose;

    protected:

        /** Closes the window immediately. 
         
            Subclasses must override the method and call parent implementation after which they must destroy the actual window which should lead to the destruction of the object (such as deleting the object in the main event loop or the UI). 
         */
        virtual void close() {
            // delete the root if attached
            Widget * rootWidget = root();
            setRoot(nullptr);
            delete rootWidget;
        };

        //@}

    protected:
        Window(int width, int height, FontMetrics const & font, EventQueue & eq):
            Renderer{Size{width, height}, eq},
            title_{"terminal++"},
            icon_{Icon::Default},
            zoom_{1.0},
            fullscreen_{false}
            /*mouseButtonsDown_{0} */ {
            // calculate the width and height in pixels and the cell dimensions from the font at given zoom level
            baseCellSize_ = Size{font.cellWidth(), font.cellHeight()};
            // TODO do I want round, or float instead? 
            cellSize_ = Size{static_cast<int>(baseCellSize_.width() * zoom_),static_cast<int>(baseCellSize_.height() * zoom_)};
            sizePx_ = Size{cellSize_.width() * width, cellSize_.height() * height};
            // set the desired fps for the renderer
            setFps(Config::Instance().renderer.fps());
        }

        virtual void windowResized(int width, int height) {
            if (width != sizePx_.width() || height != sizePx_.height()) {
                sizePx_ = Size{width, height};
                // tell the renderer to resize 
                resize(Size{width / cellSize_.width(), height / cellSize_.height()});
            }
        }

        /** Converts the x & y coordinates in pixels to cell coordinates. 
         */
        Point pixelsToCoords(Point xy) {
            int px = xy.x() / cellSize_.width();
            int py = xy.y() / cellSize_.height();
            // there is no -0, so coordinates smaller than 0 are immediately at least -1
            if (xy.x() < 0)
              --px;
            if (xy.y() < 0)
              --py;
            return Point{px, py};
        }

        /** \name Mouse Input
         
            The coordinates reported to the renderer are in pixels and must be converted to terminal columns and rows before they are passed further. 

            TODO do I need these?
         */
        //@{
        void mouseDown(Point coords, MouseButton button) override {
            ++mouseButtonsDown_;
            Renderer::mouseDown(coords, button);
        }

        void mouseUp(Point coords, MouseButton button) override {
            if (mouseButtonsDown_ > 0) 
                --mouseButtonsDown_;
            Renderer::mouseUp(coords, button);
        }

        //@}

        /** Title of the window. 
         */
        std::string title_;
        Icon icon_;

        Size sizePx_;

        Size baseCellSize_;

        Size cellSize_;

        double zoom_;

        bool fullscreen_;

        /** Mouse buttons that are currently down so that we know when to release the mouse capture. */
        unsigned mouseButtonsDown_ = 0;

    }; // tpp::Window

    /** Templated child of the Window that provides support for fast rendering via CRTP. 
     */
    template<typename IMPLEMENTATION, typename NATIVE_HANDLE>
    class RendererWindow : public Window {
    public:

        void setZoom(double value) override {
            if (value != zoom_) {
                Window::setZoom(value);
                // get the font dimensions 
                typename IMPLEMENTATION::Font * f = IMPLEMENTATION::Font::Get(ui::Font(), static_cast<int>(baseCellSize_.height() * zoom_));
                cellSize_ = Size{f->cellWidth(), f->cellHeight()};
                // tell the renderer to resize
                resize(Size{sizePx_.width() / cellSize_.width(), sizePx_.height() / cellSize_.height()});
            }
        }

    protected:

        /** Creates new renderer window. 
         
            Initializes the font metrics and event queue tied to the implementation type (DirectWrite, X11, etc.).
         */
        RendererWindow(int width, int height, EventQueue & eventQueue):
            Window{width, height, * IMPLEMENTATION::Font::Get(ui::Font(), tpp::Config::Instance().renderer.font.size()), eventQueue},
            lastCursorPos_{-1,-1} {
        }

        Cell state_;
        Point lastCursorPos_;

        static IMPLEMENTATION * GetWindowForHandle(NATIVE_HANDLE handle) {
            ASSERT(GlobalState_ != nullptr);
            std::lock_guard<std::mutex> g(GlobalState_->mWindows);
            auto i = GlobalState_->windows.find(handle);
            return i == GlobalState_->windows.end() ? nullptr : i->second;
        } 

        static void RegisterWindowHandle(IMPLEMENTATION * window, NATIVE_HANDLE handle) {
            ASSERT(GlobalState_ != nullptr);
            std::lock_guard<std::mutex> g(GlobalState_->mWindows);
            ASSERT(GlobalState_->windows.find(handle) == GlobalState_->windows.end());
            GlobalState_->windows.insert(std::make_pair(handle, window));
        }

        /** Removes the window with given handle from the list of windows. 
         */
        static void UnregisterWindowHandle(NATIVE_HANDLE handle) {
            ASSERT(GlobalState_ != nullptr);
            std::lock_guard<std::mutex> g(GlobalState_->mWindows);
            GlobalState_->windows.erase(handle);
        }

        static bool BlinkVisible() {
            ASSERT(GlobalState_ != nullptr);
            return GlobalState_->blinkVisible;
        }

        static unsigned BlinkSpeed() {
            ASSERT(GlobalState_ != nullptr);
            return GlobalState_->blinkSpeed;
        }

        static std::unordered_map<NATIVE_HANDLE, IMPLEMENTATION*> const Windows() {
            ASSERT(GlobalState_ != nullptr);
            return GlobalState_->windows;
        }


        /** Starts the blinker thread that runs for the duration of the application and periodically repaints all windows so that blinking text is properly displayed. 
         
            The method must be called by the Application instance startup.  
         */
        static void StartBlinkerThread() {
            GlobalState_ = new GlobalState{};
            std::thread t([](){
                GlobalState_->blinkVisible = true;
                while (true) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(GlobalState_->blinkSpeed));
                    GlobalState_->blinkVisible = ! GlobalState_->blinkVisible;
                    {
                        std::lock_guard<std::mutex> g(GlobalState_->mWindows);
                        for (auto i : GlobalState_->windows)
                            i.second->repaint();
                    }
                }
            });
            t.detach();
        }

        /** Global state for the window management and rendering. 
         
            Because the blinker thread is detached, the global state must be heap allocated so that the objects here are never deallocated in case the blinker thread will execute after the main function ends. 
         */
        struct GlobalState {
            /** A map which points from the native handles to the windows. */
            std::unordered_map<NATIVE_HANDLE, IMPLEMENTATION *> windows;
            /** Guard for the list of windows (ui thread and the blinker thread). */
            std::mutex mWindows;
            /** Current visibility of the blinking text. */
            std::atomic<bool> blinkVisible;
            /** The speed of the blinking text, same for all windows in the application. */
            unsigned blinkSpeed = DEFAULT_BLINK_SPEED;
        }; // tpp::RendererWindow::BlinkInfo

        static GlobalState * GlobalState_;

        #define initializeDraw(...) static_cast<IMPLEMENTATION*>(this)->initializeDraw(__VA_ARGS__)
        #define initializeGlyphRun(...) static_cast<IMPLEMENTATION*>(this)->initializeGlyphRun(__VA_ARGS__)
        #define addGlyph(...) static_cast<IMPLEMENTATION*>(this)->addGlyph(__VA_ARGS__)
        #define changeFont(...) static_cast<IMPLEMENTATION*>(this)->changeFont(__VA_ARGS__)
        #define changeFg(...) static_cast<IMPLEMENTATION*>(this)->changeForegroundColor(__VA_ARGS__)
        #define changeBg(...) static_cast<IMPLEMENTATION*>(this)->changeBackgroundColor(__VA_ARGS__)
        #define changeDecor(...) static_cast<IMPLEMENTATION*>(this)->changeDecorationColor(__VA_ARGS__)
        #define drawGlyphRun(...) static_cast<IMPLEMENTATION*>(this)->drawGlyphRun(__VA_ARGS__)
        #define drawBorder(...) static_cast<IMPLEMENTATION*>(this)->drawBorder(__VA_ARGS__)
        #define finalizeDraw(...) static_cast<IMPLEMENTATION*>(this)->finalizeDraw(__VA_ARGS__)

        using Renderer::render;

        void render(Rect const & rect) override {
            MARK_AS_UNUSED(rect);
            // then actually render the entire window
            Stopwatch t;
            t.start();
            // shorthand to the buffer
            Buffer const & buffer = this->buffer();
            // initialize the drawing and set the state for the first cell
            initializeDraw();
            state_ = buffer.at(0,0);
            changeFont(state_.font());
            changeFg(state_.fg());
            changeBg(state_.bg());
            changeDecor(state_.decor());
            // loop over the buffer and draw the cells
            for (int row = 0, re = height(); row < re; ++row) {
                initializeGlyphRun(0, row);
                for (int col = 0, ce = width(); col < ce; ) {
                    Cell const & c = buffer.at(col, row);
                    // detect if there were changes in the font & colors and update the state & draw the glyph run if present. The code looks a bit ugly as we have to first draw the glyph run and only then change the state.
                    bool drawRun = true;
                    if (state_.font() != c.font()) {
                        if (drawRun) {
                            drawGlyphRun();
                            initializeGlyphRun(col, row);
                            drawRun = false;
                        }
                        changeFont(c.font());
                        state_.setFont(c.font());
                    }
                    if (state_.fg() != c.fg()) {
                        if (drawRun) {
                            drawGlyphRun();
                            initializeGlyphRun(col, row);
                            drawRun = false;
                        }
                        changeFg(c.fg());
                        state_.setFg(c.fg());
                    }
                    if (state_.bg() != c.bg()) {
                        if (drawRun) {
                            drawGlyphRun();
                            initializeGlyphRun(col, row);
                            drawRun = false;
                        }
                        changeBg(c.bg());
                        state_.setBg(c.bg());
                    }
                    if (state_.decor() != c.decor()) {
                        if (drawRun) {
                            drawGlyphRun();
                            initializeGlyphRun(col, row);
                            drawRun = false;
                        }
                        changeDecor(c.decor());
                        state_.setDecor(c.decor());
                    }
                    // we don't care about the border at this stage
                    // draw the cell
                    addGlyph(col, row, c);
                    // move to the next column (skip invisible cols if double width or larger font)
                    col += c.font().width();
                }
                drawGlyphRun();
            }
            
            // determine the cursor, its visibility and its position and draw it if necessary. The cursor is drawn when it is not blinking, when its position has changed since last time it was drawn with blink on or if it is blinking and blink is visible. This prevents the cursor for disappearing while moving
            Point cursorPos = buffer.cursorPosition();
            Canvas::Cursor cursor = buffer.cursor();
            if (buffer.contains(cursorPos) && cursor.visible() && (! cursor.blink() || BlinkVisible() || cursorPos != lastCursorPos_)) {
                state_.setCodepoint(cursor.codepoint());
                state_.setFg(cursor.color());
                state_.setBg(Color::None);
                state_.setFont(buffer.at(cursorPos).font());
                changeFont(state_.font());
                changeFg(state_.fg());
                changeBg(state_.bg());
                initializeGlyphRun(cursorPos.x(), cursorPos.y());
                addGlyph(cursorPos.x(), cursorPos.y(), state_);
                drawGlyphRun();
                if (BlinkVisible())
                    lastCursorPos_ = cursorPos;
            }

            // finally, draw the border, which is done on the base cell level over the already drawn text
            int wThin = std::min(cellSize_.width(), cellSize_.height()) / 4;
            int wThick = std::min(cellSize_.width(), cellSize_.height()) / 2;
            Color borderColor = buffer.at(0,0).border().color();
            changeBg(borderColor);
            for (int row = 0, re = height(); row < re; ++row) {
                for (int col = 0, ce = width(); col < ce; ++col) {
                    Border b = buffer.at(col, row).border();
                    if (b.color() != borderColor) {
                        borderColor = b.color();
                        changeBg(borderColor);
                    }
                    if (! b.empty())
                        drawBorder(col, row, b, wThin, wThick);
                }
            }

            finalizeDraw();
        }

        #undef initializeDraw
        #undef initializeGlyphRun
        #undef addGlyph
        #undef changeFont
        #undef changeFg
        #undef changeBg
        #undef changeDecor
        #undef changeBorderColor
        #undef drawGlyphRun
        #undef drawBorder
        #undef finalizeDraw

    }; // tpp::RendererWindow

    template<typename IMPLEMENTATION, typename NATIVE_HANDLE>
    typename tpp::RendererWindow<IMPLEMENTATION, NATIVE_HANDLE>::GlobalState * tpp::RendererWindow<IMPLEMENTATION, NATIVE_HANDLE>::GlobalState_ = nullptr;

} // namespace tpp

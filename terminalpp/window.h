#pragma once

#include <algorithm>
#include <thread>
#include <mutex>

#include "helpers/time.h"

#include "ui/canvas.h"

#include "ui/renderer.h"

#include "config.h"

#include "application.h"
#include "font.h"

namespace tpp {

    using namespace ui;

    /** Base class for displaying an UI window content. 
     
        Builds upon the local renderer and adds the basic properties required for GUI windows, such as width and height in pixels, etc.  
     */
    class Window: public LocalRenderer {
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

        int widthPx() const {
            return widthPx_;
        }

        int heightPx() const {
            return heightPx_;
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

        /** Requests the renderer window to be closed. 
         */
        using Renderer::requestClose;

    protected:
        Window(int width, int height, FontMetrics const & font, double zoom):
            LocalRenderer{width, height},
            title_{"terminal++"},
            icon_{Icon::Default},
            zoom_{zoom},
            fullscreen_{false},
            activeModifiers_{0},
            mouseButtonsDown_{0} {
            // calculate the width and height in pixels and the cell dimensions from the font at given zoom level
            baseCellWidth_ = font.cellWidth();
            baseCellHeight_ = font.cellHeight();
            // TODO do I want round, or float instead? 
            cellWidth_ = static_cast<int>(baseCellWidth_ * zoom);
            cellHeight_ = static_cast<int>(baseCellHeight_ * zoom);
            widthPx_ = cellWidth_ * width;
            heightPx_ = cellHeight_ * height;
        }

        virtual void windowResized(int width, int height) {
            if (width != widthPx_ || height != heightPx_) {
                widthPx_ = width;
                heightPx_ = height;
                // tell the renderer to resize 
                resize(width / cellWidth_, height / cellHeight_);
            }
        }

        /** Converts the x & y coordinates in pixels to cell coordinates. 
         */
        Point pixelsToCoords(Point xy) {
            int px = xy.x() / cellWidth_;
            int py = xy.y() / cellHeight_;
            // there is no -0, so coordinates smaller than 0 are immediately at least -1
            if (xy.x() < 0)
              --px;
            if (xy.y() < 0)
              --py;
            return Point{px, py};
        }

        /** \name Renderer API
         
            The coordinates reported to the renderer are in pixels and must be converted to terminal columns and rows before they are passed further. 
         */
        //@{
        void rendererMouseMove(Point coords, Key modifiers) override {
            LocalRenderer::rendererMouseMove(pixelsToCoords(coords), modifiers);
        }

        void rendererMouseDown(Point coords, MouseButton button, Key modifiers) override {
            ++mouseButtonsDown_; 
            LocalRenderer::rendererMouseDown(pixelsToCoords(coords), button, modifiers);
        }

        void rendererMouseUp(Point coords, MouseButton button, Key modifiers) override {
            if (mouseButtonsDown_ > 0) {
                --mouseButtonsDown_;
                LocalRenderer::rendererMouseUp(pixelsToCoords(coords), button, modifiers);
            }
        }

        void rendererMouseWheel(Point coords, int by, Key modifiers) override {
            LocalRenderer::rendererMouseWheel(pixelsToCoords(coords), by, modifiers);
        }

        /** A more relaxed version of mouse out event.

            Mouse leave translates to mouseOut in cases when the mouse is not captured, i.e. no buttons are currently down.  
         */
        void rendererMouseLeave() {
            if (mouseButtonsDown_ == 0)
                rendererMouseOut();
        }

        //@}

        /** \name Global events
         */
        //@{

        void keyDown(Event<Key>::Payload & e, Widget * target) override {
            if (*e == SHORTCUT_FULLSCREEN) {
                setFullscreen(! fullscreen_);
                e.stop();
            } else if (*e == SHORTCUT_SETTINGS) {
                Application::Instance()->openLocalFile(Config::GetSettingsFile(), /* edit = */ true);
                e.stop();
            } else if (*e == SHORTCUT_ZOOM_IN) {
                if (zoom_ < 10)
                    setZoom(zoom_ * 1.25);
                e.stop();
            } else if (*e == SHORTCUT_ZOOM_OUT) {
                if (zoom() > 1)
                    setZoom(std::max(1.0, zoom() / 1.25));
                e.stop();
            }   
            LocalRenderer::keyDown(e, target);
        }

        //@}

        /** Title of the window. 
         */
        std::string title_;
        Icon icon_;

        int widthPx_;
        int heightPx_;

        int baseCellWidth_;
        int baseCellHeight_;

        int cellWidth_;
        int cellHeight_;

        double zoom_;

        bool fullscreen_;

        ui::Key activeModifiers_;

        /** Mouse buttons that are currently down so that we know when to release the mouse capture. */
        unsigned mouseButtonsDown_;
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
                typename IMPLEMENTATION::Font * f = IMPLEMENTATION::Font::Get(ui::Font(), static_cast<int>(baseCellHeight_ * zoom_));
                cellWidth_ = f->cellWidth();
                cellHeight_ = f->cellHeight();
                // tell the renderer to resize
                resize(widthPx_ / cellWidth_, heightPx_ / cellHeight_);
            }
        }

    protected:
        RendererWindow(int width, int height, FontMetrics const & font, double zoom):
            Window{width, height, font, zoom},
            lastCursorPos_{-1,-1} {
        }

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
            // get constant reference so that reading does not change the unused bits in the cells we use for cursor
            Buffer const & buf{buffer()};
            // then actually render the entire window
            helpers::Stopwatch t;
            t.start();
            // initialize the drawing and set the state for the first cell
            initializeDraw();
            state_ = buf.at(0,0);
            changeFont(state_.font());
            changeFg(state_.fg());
            changeBg(state_.bg());
            changeDecor(state_.decor());
            // loop over the buffer and draw the cells
            for (int row = 0, re = height(); row < re; ++row) {
                initializeGlyphRun(0, row);
                for (int col = 0, ce = width(); col < ce; ) {
                    Cell const & c = buf.at(col, row);
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
            ui::Cursor cursor = buf.cursor();
            Point cursorPos = buf.cursorPosition();
            if (cursor.visible() && (! cursor.blink() || BlinkVisible() || cursorPos != lastCursorPos_)) {
                state_.setCodepoint(cursor.codepoint())
                      .setFg(cursor.color())
                      .setBg(Color::None)
                      .setFont(buf.at(cursorPos).font());
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
            int wThin = std::min(cellWidth_, cellHeight_) / 4;
            int wThick = std::min(cellWidth_, cellHeight_) / 2;
            Color borderColor = buf.at(0,0).border().color();
            changeBg(borderColor);
            for (int row = 0, re = height(); row < re; ++row) {
                for (int col = 0, ce = width(); col < ce; ++col) {
                    Border b = buf.at(col, row).border();
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
                            i.second->repaint(nullptr);
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

    }; // tpp::RendererWindow

    template<typename IMPLEMENTATION, typename NATIVE_HANDLE>
    typename tpp::RendererWindow<IMPLEMENTATION, NATIVE_HANDLE>::GlobalState * tpp::RendererWindow<IMPLEMENTATION, NATIVE_HANDLE>::GlobalState_ = nullptr;

} // namespace tpp

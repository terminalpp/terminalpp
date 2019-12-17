#pragma once

#include <algorithm>
#include <thread>
#include <mutex>

#include "helpers/time.h"

#include "ui/canvas.h"
#include "ui/renderer.h"

#include "config.h"

#include "application.h"

namespace tpp {

    /** Base class for display a UI window contents and capturing the mouse, keyboard and clipboard events. 
     */
    class Window : public ui::Renderer {
    public:

        using ui::Renderer::rootWindow;
        using ui::Renderer::setRootWindow;

        virtual void show() = 0;
        virtual void hide() = 0;

        /** Shorthand to repaint the entire window. 
         */
        void repaint() {
            requestRender(ui::Rect::FromWH(cols_, rows_));
        }

        int cols() const override {
            return cols_;
        }

        int rows() const override {
            return rows_;
        }

        bool fullscreen() const {
            return fullscreen_;
        }

        void setFullscreen(bool value) {
            if (fullscreen_ != value)
                updateFullscreen(value);
        };

        double zoom() const {
            return zoom_;
        }

        void setZoom(double value) {
            if (zoom_ != value)
                updateZoom(value);
        }

    protected:

		Window(int cols, int rows, unsigned cellWidthPx, unsigned cellHeightPx) :
			cols_(cols),
			rows_(rows),
			widthPx_(cols* cellWidthPx),
			heightPx_(rows* cellHeightPx),
            baseCellHeightPx_(cellHeightPx),
			cellWidthPx_(cellWidthPx),
			cellHeightPx_(cellHeightPx),
			zoom_(1.0),
			fullscreen_(false),
            mouseButtonsDown_(0),
            blinkVisible_(true),
            cursorBlinkVisible_(true) {
		}

        /** The actual paint method. 
         */
        virtual void paint() = 0;

        virtual void setFocus(bool value) {
            // if the window is loosing its focus, reset active modifiers (otherwise things like dangling ALT from ALT-TAB app switch are possible) 
            if (! value)
                activeModifiers_ = ui::Key::Invalid;
            ui::Renderer::setFocus(value);
        }

        /** Converts the x & y coordinates in pixels to cell coordinates. 
         */
		void convertMouseCoordsToCells(int & x, int & y) {
			x = x / cellWidthPx_;
			y = y / cellHeightPx_;
		}


        virtual void updateSizePx(unsigned widthPx, unsigned heightPx) {
            widthPx_ = widthPx;
            heightPx_ = heightPx;
            updateSize(widthPx_ / cellWidthPx_, heightPx_ / cellHeightPx_);
        }

        virtual void updateSize(int cols, int rows) {
            cols_ = cols;
            rows_ = rows;
            // notify the rendered root window
            ui::Renderer::updateSize(cols, rows);
        }

        virtual void updateFullscreen(bool value) {
            fullscreen_ = value;
        }

        virtual void updateZoom(double value) {
            zoom_ = value;
        }

        // interface to ui's root element

        virtual void mouseDown(int x, int y, ui::MouseButton button) {
            ++mouseButtonsDown_;
            convertMouseCoordsToCells(x, y);
            ui::Renderer::mouseDown(x, y, button, activeModifiers_);
        }

        virtual void mouseUp(int x, int y, ui::MouseButton button) {
            ASSERT(mouseButtonsDown_ > 0);
            --mouseButtonsDown_;
            convertMouseCoordsToCells(x, y);
            ui::Renderer::mouseUp(x, y, button, activeModifiers_);
        }

        virtual void mouseWheel(int x, int y, int by) {
            convertMouseCoordsToCells(x, y);
            ui::Renderer::mouseWheel(x, y, by, activeModifiers_);
        }

        virtual void mouseMove(int x, int y) {
            convertMouseCoordsToCells(x, y);
            ui::Renderer::mouseMove(x, y, activeModifiers_);
        }

        virtual void keyChar(helpers::Char c) {
            ui::Renderer::keyChar(c);
        }

        virtual void keyDown(ui::Key key) {
    		activeModifiers_ = ui::Key(ui::Key::Invalid, key.modifiers());
            // TODO make these nicer actions
            if (key == SHORTCUT_FULLSCREEN) {
                setFullscreen(!fullscreen());
            // open settings in editor
            } else if (key == SHORTCUT_SETTINGS) {
                Config::OpenSettingsInEditor();
            // zoom in
            } else if (key == SHORTCUT_ZOOM_IN) {
                if (zoom() < 10)
                    setZoom(zoom() * 1.25);
            // zoom out
            } else if (key == SHORTCUT_ZOOM_OUT) {
                if (zoom() > 1)
                    setZoom(std::max(1.0, zoom() / 1.25));
            } else if (key != ui::Key::Invalid) {
                ui::Renderer::keyDown(key);
            }
        }

        virtual void keyUp(ui::Key key) {
    		activeModifiers_ = ui::Key(ui::Key::Invalid, key.modifiers());
            if (key != ui::Key::Invalid)
                ui::Renderer::keyUp(key);
        }

        /** Sets the window's blink visibility to given value. 
         
            If the visibility is different to the current one also triggers the redraw of the entire window. 
         */
        void setBlinkVisible(bool value) {
            if (blinkVisible_ != value) {
                blinkVisible_ = value;
                if (blinkVisible_ != cursorBlinkVisible_)
                    cursorBlinkVisible_ = blinkVisible_;
                repaint();
            } 
        }

		int cols_;
		int rows_;
		unsigned widthPx_;
        unsigned heightPx_;
        unsigned baseCellHeightPx_;
        int cellWidthPx_;
        int cellHeightPx_;

        double zoom_;

        bool fullscreen_;

        ui::Key activeModifiers_;

        /* Number of mouse buttons currently pressed so that we know when to set & release mouse capture.
         */
        unsigned mouseButtonsDown_;

        /* Determines if the blinking text and cursor are currently visible, or not. 
         */
        bool blinkVisible_;
        bool cursorBlinkVisible_;

        /** Determines the blink speed in milliseconds. 
         */
        static unsigned BlinkSpeed_;

    };

    template<typename IMPLEMENTATION, typename NATIVE_HANDLE> 
    class RendererWindow : public Window {
    protected:

		RendererWindow(int cols, int rows, unsigned cellWidthPx, unsigned baseCellHeightPx) :
            Window(cols, rows, cellWidthPx, baseCellHeightPx),
            painting_{0} {
        }

        /** Draws the provided buffer in the window. 
         */
        void paint() override {
            if (!attached())
                return;
            // see if we are the first one to paint, terminate if not
            if (++painting_ != 1)
                return;
            #define initializeDraw(...) reinterpret_cast<IMPLEMENTATION*>(this)->initializeDraw(__VA_ARGS__)
            #define initializeGlyphRun(...) reinterpret_cast<IMPLEMENTATION*>(this)->initializeGlyphRun(__VA_ARGS__)
            #define addGlyph(...) reinterpret_cast<IMPLEMENTATION*>(this)->addGlyph(__VA_ARGS__)
            #define setFont(...) reinterpret_cast<IMPLEMENTATION*>(this)->setFont(__VA_ARGS__)
            #define setForegroundColor(...) reinterpret_cast<IMPLEMENTATION*>(this)->setForegroundColor(__VA_ARGS__)
            #define setBackgroundColor(...) reinterpret_cast<IMPLEMENTATION*>(this)->setBackgroundColor(__VA_ARGS__)
            #define setDecorationColor(...) reinterpret_cast<IMPLEMENTATION*>(this)->setDecorationColor(__VA_ARGS__)
            #define setBorderColor(...) reinterpret_cast<IMPLEMENTATION*>(this)->setBorderColor(__VA_ARGS__)
            #define setAttributes(...) reinterpret_cast<IMPLEMENTATION*>(this)->setAttributes(__VA_ARGS__)
            #define drawGlyphRun(...) reinterpret_cast<IMPLEMENTATION*>(this)->drawGlyphRun(__VA_ARGS__)
            #define drawBorder(...) reinterpret_cast<IMPLEMENTATION*>(this)->drawBorder(__VA_ARGS__)
            #define finalizeDraw(...) reinterpret_cast<IMPLEMENTATION*>(this)->finalizeDraw(__VA_ARGS__)

            helpers::Stopwatch t;
            t.start();
            {
                initializeDraw();
                // Lock the buffer so that the drawing method has exclusive access
                ui::Canvas::Buffer::Ptr buffer = bufferToRender();
                // reset the status cell and call selectors on the font, colors and attributes
                statusCell_ = buffer->at(0,0);
                setFont(statusCell_.font());
                setForegroundColor(statusCell_.foreground());
                setBackgroundColor(statusCell_.background());
                setDecorationColor(statusCell_.decorationColor());
                setAttributes(statusCell_.attributes()); 
                for (int row = 0, re = std::min(rows_, buffer->rows()); row < re; ++row) {
                    initializeGlyphRun(0, row);
                    for (int col = 0, ce = std::min(cols_, buffer->cols()); col < ce;) {
                        // get the cell to be drawn
                        ui::Cell const & c = buffer->at(col, row);
                        // now we know the cell must be drawn, determine if the attributes of the cell changed since last cell drawn
                        if ((statusCell_ << c.codepoint()) != c) {
                            drawGlyphRun();
                            initializeGlyphRun(col, row);
                            if (statusCell_.font() != c.font())
                                setFont(c.font());
                            if (statusCell_.foreground() != c.foreground())
                                setForegroundColor(c.foreground());
                            if (statusCell_.background() != c.background())
                                setBackgroundColor(c.background());
                            if (statusCell_.decorationColor() != c.decorationColor())
                                setDecorationColor(c.decorationColor());
                            if (statusCell_.attributes() != c.attributes())
                                setAttributes(c.attributes());
                            statusCell_ = c;
                        }
                        // draw the cell
                        addGlyph(col, row, c);
                        // move to the next column (skip invisible cols if double width or larger font)
                        col += c.font().width();
                    }
                    drawGlyphRun();
                }
                // draw the cursor by creating a cell corresponding to how the cursor should be displayed
                ui::Cursor const & cursor = cursorToRender();
                // it is possible that cursor is outside of the terminal window, in which case it is not rendered
                if (cursor.visible == true && cursor.pos.x < buffer->cols() && cursor.pos.y < buffer->rows() && buffer->at(cursor.pos).isCursor()) {
                    if (cursor.blink && cursor.pos != lastCursorPosition_)
                        cursorBlinkVisible_ = true;
                    if (! cursor.blink || cursorBlinkVisible_) {
                        initializeGlyphRun(cursor.pos.x, cursor.pos.y);
                        statusCell_ << cursor.codepoint 
                                << ui::Foreground(cursor.color)
                                << ui::Background(ui::Color::None) 
                                << buffer->at(cursor.pos).font()
                                << ui::Attributes();
                        setFont(statusCell_.font());
                        setForegroundColor(statusCell_.foreground());
                        setBackgroundColor(ui::Color::None);
                        setDecorationColor(statusCell_.decorationColor());
                        setAttributes(statusCell_.attributes()); 
                        addGlyph(cursor.pos.x, cursor.pos.y, statusCell_);
                        drawGlyphRun();
                    }
                }
                lastCursorPosition_ = cursor.pos;
                // finally, draw the border, which is done on the base cell level over the already drawn text
                statusCell_ = buffer->at(0,0);
                setBorderColor(statusCell_.borderColor());
                int wThin = std::min(cellWidthPx_, cellHeightPx_) / 4;
                int wThick = std::min(cellWidthPx_, cellHeightPx_) / 2;
                for (int row = 0, re = std::min(rows_, buffer->rows()); row < re; ++row) {
                    for (int col = 0, ce = std::min(cols_, buffer->cols()); col < ce; ++col) {
                        ui::Cell const & c = buffer->at(col, row);
                        if (statusCell_.borderColor() != c.borderColor()) {
                            statusCell_ << ui::BorderColor(c.borderColor());
                            setBorderColor(statusCell_.borderColor());
                        }
                        ui::Attributes attrs = c.attributes();
                        if (attrs.border()) {
                            if (attrs.borderThick() && attrs.borderLeft() && attrs.borderRight()) {
                                // We don't expect width to be greater than height, in which case the left & top should instead be top & bottom...
                                ASSERT(cellWidthPx_ <= cellHeightPx_);
                                attrs.setBorderRight(false).setBorderTop(false).setBorderBottom(false);
                                drawBorder(attrs, col * cellWidthPx_, row * cellHeightPx_, cellWidthPx_);
                            } else {
                                drawBorder(attrs, col * cellWidthPx_, row * cellHeightPx_, attrs.borderThick() ? wThick : wThin);
                            }

                        }
                    }
                }
                finalizeDraw();
            }
			t.stop();
            // if there were any paint requests while we were painting, request a repaint
            if (--painting_ != 0) {
                painting_ = 0;
                requestRender(ui::Rect::FromWH(cols_, rows_));
            }
            
			//std::cout << "Paint: " << t.value() << " ms\n" ;
            // and we are done
            #undef initializeDraw
            #undef initializeGlyphRun
            #undef addGlyph
            #undef setFont
            #undef setForegroundColor
            #undef setBackgroundColor
            #undef setDecorationColor
            #undef setBorderColor
            #undef setAttributes
            #undef drawGlyphRun
            #undef drawBorder
            #undef finalizeDraw
        }

        /** Mutex guarding the paint method, making sure only one paint is executed and if paint requests are received while painting, they will be buffered and new paint issued to them after current paint finishes. 
         */
        std::atomic<unsigned> painting_;

        ui::Cell statusCell_;

        ui::Point lastCursorPosition_;

        static IMPLEMENTATION * GetWindowFromNativeHandle(NATIVE_HANDLE handle) {
            std::lock_guard<std::mutex> g(MWindows_);
            auto i = Windows_.find(handle);
            return i == Windows_.end() ? nullptr : i->second;
        }

        static void AddWindowNativeHandle(IMPLEMENTATION * window, NATIVE_HANDLE handle) {
            std::lock_guard<std::mutex> g(MWindows_);
            ASSERT(Windows_.find(handle) == Windows_.end());
            Windows_.insert(std::make_pair(handle, window));
        }

        /** Removes the window with given handle from the list of windows. 
         
            Returns false if there are any windows left, true if this was the last window in the list.
         */
        static bool RemoveWindow(NATIVE_HANDLE handle) {
            std::lock_guard<std::mutex> g(MWindows_);
            Windows_.erase(handle);
            return Windows_.empty();
        }

        static void StartBlinkerThread() {
            std::thread t([](){
                bool blink = true;
                while (true) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(BlinkSpeed_));
                    blink = ! blink;
                    std::lock_guard<std::mutex> g(MWindows_);
                    for (auto i : Windows_)
                        i.second->setBlinkVisible(blink);
                }
            });
            t.detach();
        }

        /** Mutex guarding the list of windows. 
         */
        static std::mutex MWindows_;

        /* A map which points from the native handles to the windows. 
         */
        static std::unordered_map<NATIVE_HANDLE, IMPLEMENTATION *> Windows_;

    };

    template<typename IMPLEMENTATION, typename NATIVE_HANDLE>
    std::mutex RendererWindow<IMPLEMENTATION, NATIVE_HANDLE>::MWindows_;

    template<typename IMPLEMENTATION, typename NATIVE_HANDLE>
    std::unordered_map<NATIVE_HANDLE, IMPLEMENTATION *> RendererWindow<IMPLEMENTATION, NATIVE_HANDLE>::Windows_;

} // namespace tpp
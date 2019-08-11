#pragma once

#include <algorithm>

#include "helpers/time.h"

#include "ui/canvas.h"
#include "ui/root_window.h"

#include "config.h"

#include "application.h"

namespace tpp {

    /** Base class for display a UI window contents and capturing the mouse, keyboard and clipboard events. 
     */
    class Window : public ui::Renderer {
    public:

        virtual ~Window() {
            detach();
        }

        virtual void show() = 0;
        virtual void hide() = 0;
        virtual void close() = 0;

        virtual void paint(ui::RectEvent & e) = 0;

        /** Shorthand to repaint the entire window. 
         */
        virtual void repaint() {
            ui::RectEvent e(nullptr, ui::Rect(cols_, rows_));
            paint(e);
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

        ui::RootWindow * rootWindow() const {
            return rootWindow_;
        }

        void setRootWindow(ui::RootWindow * rootWindow) {
            if (rootWindow_ != rootWindow) {
                detach();
                attach(rootWindow);
            }
        }


    protected:

		Window(std::string const & title, int cols, int rows, unsigned cellWidthPx, unsigned cellHeightPx) :
			cols_(cols),
			rows_(rows),
			widthPx_(cols* cellWidthPx),
			heightPx_(rows* cellHeightPx),
            baseCellHeightPx_(cellHeightPx),
			cellWidthPx_(cellWidthPx),
			cellHeightPx_(cellHeightPx),
			zoom_(1.0),
			fullscreen_(false),
            title_(title),
            rootWindow_(nullptr) {
		}

        void detach() {
            if (rootWindow_ == nullptr)
                return;
            rootWindow_->rendererDetached(this);
            rootWindow_->onRepaint -= HANDLER(Window::paint, this);
            rootWindow_ = nullptr;
        }

        void attach(ui::RootWindow * rootWindow) {
            if (rootWindow == nullptr)
                return;
            ASSERT(rootWindow_ == nullptr);
            rootWindow_ = rootWindow;
            rootWindow_->onRepaint += HANDLER(Window::paint, this);
            rootWindow_->rendererAttached(this);
            rootWindow_->rendererResized(this, cols_, rows_);
        }

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
            if (rootWindow_)
                rootWindow_->rendererResized(this, cols_, rows_);
        }

        virtual void updateFullscreen(bool value) {
            fullscreen_ = value;
        }

        virtual void updateZoom(double value) {
            zoom_ = value;
        }

        // interface to ui's root element

        virtual void mouseDown(int x, int y, ui::MouseButton button) {
            if (rootWindow_) {
                convertMouseCoordsToCells(x, y);
                rootWindow_->mouseDown(x, y, button, activeModifiers_);
            }
        }

        virtual void mouseUp(int x, int y, ui::MouseButton button) {
            if (rootWindow_) {
                convertMouseCoordsToCells(x, y);
                rootWindow_->mouseUp(x, y, button, activeModifiers_);
            }
        }

        virtual void mouseWheel(int x, int y, int by) {
            if (rootWindow_) {
                convertMouseCoordsToCells(x, y);
                rootWindow_->mouseWheel(x, y, by, activeModifiers_);
            }
        }

        virtual void mouseMove(int x, int y) {
            if (rootWindow_) {
                convertMouseCoordsToCells(x, y);
                rootWindow_->mouseMove(x, y, activeModifiers_);
            }
        }

        virtual void keyChar(helpers::Char c) {
            if (rootWindow_)
                rootWindow_->keyChar(c);
        }

        virtual void keyDown(ui::Key key) {
    		activeModifiers_ = ui::Key(ui::Key::Invalid, key.modifiers());
            // TODO make these nicer actions
            if (key == SHORTCUT_FULLSCREEN) {
                setFullscreen(!fullscreen());
            // zoom in
            } else if (key == SHORTCUT_ZOOM_IN) {
                if (zoom() < 10)
                    setZoom(zoom() * 1.25);
            // zoom out
            } else if (key == SHORTCUT_ZOOM_OUT) {
                if (zoom() > 1)
                    setZoom(std::max(1.0, zoom() / 1.25));
            } else if (key == SHORTCUT_PASTE) {
                requestClipboardPaste();
            } else if (key != ui::Key::Invalid && rootWindow_) {
                rootWindow_->keyDown(key);
            }
        }

        virtual void keyUp(ui::Key key) {
    		activeModifiers_ = ui::Key(ui::Key::Invalid, key.modifiers());
            if (key != ui::Key::Invalid && rootWindow_)
                rootWindow_->keyUp(key);
        }

        virtual void requestClipboardPaste() = 0;

        void paste(std::string const & clipboard) {
            rootWindow_->paste(clipboard);
        }


		int cols_;
		int rows_;
		unsigned widthPx_;
        unsigned heightPx_;
        unsigned baseCellHeightPx_;
        unsigned cellWidthPx_;
        unsigned cellHeightPx_;

        double zoom_;

        bool fullscreen_;

        std::string title_;

        ui::RootWindow * rootWindow_;

        ui::Key activeModifiers_;
    };

    template<typename IMPLEMENTATION> 
    class RendererWindow : public Window {
    protected:

		RendererWindow(std::string const & title, int cols, int rows, unsigned cellWidthPx, unsigned baseCellHeightPx) :
            Window(title, cols, rows, cellWidthPx, baseCellHeightPx) {
        }

		void render() {
			if (rootWindow_ != nullptr)
				render(rootWindow_);
		}

        /** Draws the provided buffer in the window. 
         */
        void render(ui::RootWindow * window) {
            #define initializeDraw(...) reinterpret_cast<IMPLEMENTATION*>(this)->initializeDraw(__VA_ARGS__)
            #define initializeGlyphRun(...) reinterpret_cast<IMPLEMENTATION*>(this)->initializeGlyphRun(__VA_ARGS__)
            #define addGlyph(...) reinterpret_cast<IMPLEMENTATION*>(this)->addGlyph(__VA_ARGS__)
            #define setFont(...) reinterpret_cast<IMPLEMENTATION*>(this)->setFont(__VA_ARGS__)
            #define setForegroundColor(...) reinterpret_cast<IMPLEMENTATION*>(this)->setForegroundColor(__VA_ARGS__)
            #define setBackgroundColor(...) reinterpret_cast<IMPLEMENTATION*>(this)->setBackgroundColor(__VA_ARGS__)
            #define setDecorationColor(...) reinterpret_cast<IMPLEMENTATION*>(this)->setDecorationColor(__VA_ARGS__)
            #define setAttributes(...) reinterpret_cast<IMPLEMENTATION*>(this)->setAttributes(__VA_ARGS__)
            #define drawGlyphRun(...) reinterpret_cast<IMPLEMENTATION*>(this)->drawGlyphRun(__VA_ARGS__)
            #define finalizeDraw(...) reinterpret_cast<IMPLEMENTATION*>(this)->finalizeDraw(__VA_ARGS__)

            helpers::Timer t;
            t.start();
            {
                initializeDraw();
                // Lock the buffer so that the drawing method has exclusive access
                ui::Canvas::Buffer::Ptr buffer = window->buffer(/* priority */ true);
                // reset the status cell and call selectors on the font, colors and attributes
                statusCell_ = buffer->at(0,0);
                setFont(statusCell_.font());
                setForegroundColor(statusCell_.foreground());
                setBackgroundColor(statusCell_.background());
                setDecorationColor(statusCell_.decorationColor());
                setAttributes(statusCell_.attributes()); 

                for (int row = 0, re = std::min(rows_, buffer->rows()); row < re; ++row) {
                    initializeGlyphRun(0, row);
                    for (int col = 0, ce = std::min(cols_, buffer->cols()); col < ce; ++col) {
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
                        addGlyph(c);
                    }
                    drawGlyphRun();
                }
                // draw the cursor by creating a cell corresponding to how the cursor should be displayed
                ui::Cursor const & cursor = window->cursor();
                if (cursor.visible == true && buffer->at(cursor.pos).isCursor()) {
                    initializeGlyphRun(cursor.pos.x, cursor.pos.y);
                    statusCell_ << cursor.codepoint 
                            << ui::Foreground(cursor.color)
                            << ui::Background(ui::Color::None()) 
                            << buffer->at(cursor.pos).font()
                            << (cursor.blink ? ui::Attributes::Blink() : ui::Attributes());
                    setFont(statusCell_.font());
                    setForegroundColor(statusCell_.foreground());
                    setBackgroundColor(ui::Color::None());
                    setDecorationColor(statusCell_.decorationColor());
                    setAttributes(statusCell_.attributes()); 
                    addGlyph(statusCell_);
                    drawGlyphRun();
                }
                finalizeDraw();
            }
			t.stop();
			std::cout << "Paint: " << t.value() << " ms\n" ;
            // and we are done
            #undef initializeDraw
            #undef initializeGlyphRun
            #undef addGlyph
            #undef setFont
            #undef setForegroundColor
            #undef setBackgroundColor
            #undef setDecorationColor
            #undef setAttributes
            #undef drawGlyphRun
            #undef finalizeDraw
        }


        ui::Cell statusCell_;
    };



} // namespace tpp
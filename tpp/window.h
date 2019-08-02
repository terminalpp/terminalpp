#pragma once

#include <algorithm>

#include "ui/canvas.h"
#include "ui/root_window.h"

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
    };

    template<typename IMPLEMENTATION> 
    class RendererWindow : public Window {
    protected:

		RendererWindow(std::string const & title, int cols, int rows, unsigned cellWidthPx, unsigned baseCellHeightPx) :
            Window(title, cols, rows, cellWidthPx, baseCellHeightPx) {
        }

		virtual void render() {
			if (rootWindow_ != nullptr)
				render(rootWindow_);
		}

        /** Draws the provided buffer in the window. 
         */
        void render(ui::RootWindow * window) {
            #define shouldDraw(...) reinterpret_cast<IMPLEMENTATION*>(this)->shouldDraw(__VA_ARGS__)
            #define drawCell(...) reinterpret_cast<IMPLEMENTATION*>(this)->drawCell(__VA_ARGS__)
            #define setFont(...) reinterpret_cast<IMPLEMENTATION*>(this)->setFont(__VA_ARGS__)
            #define setForegroundColor(...) reinterpret_cast<IMPLEMENTATION*>(this)->setForegroundColor(__VA_ARGS__)
            #define setBackgroundColor(...) reinterpret_cast<IMPLEMENTATION*>(this)->setBackgroundColor(__VA_ARGS__)
            #define setDecorationColor(...) reinterpret_cast<IMPLEMENTATION*>(this)->setDecorationColor(__VA_ARGS__)
            #define setAttributes(...) reinterpret_cast<IMPLEMENTATION*>(this)->setAttributes(__VA_ARGS__)

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
                for (int col = 0, ce = std::min(cols_, buffer->cols()); col < ce; ++col) {
                    // get the cell to be drawn
                    ui::Cell const & c = buffer->at(col, row);
                    // determine if the cell needs to be redrawn
                    if (! shouldDraw(col, row, c)) 
                        continue;
                    // now we know the cell must be drawn, determine if the attributes of the cell changed since last cell drawn
                    if ((statusCell_ << c.codepoint()) != c) {
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
                    drawCell(col, row, c);
                }
            }
            // draw the cursor by creating a cell corresponding to how the cursor should be displayed
            // and we are done
            #undef initializeDraw
            #undef shouldDraw
            #undef drawCell
            #undef setFont
            #undef setForegroundColor
            #undef setBackgroundColor
            #undef setDecorationColor
            #undef setAttributes
        }

        ui::Cell statusCell_;
    };



} // namespace tpp
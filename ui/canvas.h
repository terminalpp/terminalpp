#pragma once

#include "buffer.h"
#include "geometry.h"

namespace ui {

    using StringLine = helpers::StringLine;

    class Widget;

    /** 
     */
    class Canvas {
    public:

        /** Finalizer function for a canvas. 
         
            Finalizer function runs when the canvas is to be destroyed, i.e. after all other painting on the canvas, which allows the finalizer to override any existing content (which is useful for drawing borders or transparent overlays). 
         */
        using Finalizer = std::function<void(Canvas&)>;

        /** Creates a canvas that encompasses the entire buffer. 
         
            Only the main UI thread can create a canvas. 
         */
        Canvas(Widget * widget);

        ~Canvas() {
            if (finalizer_)
                finalizer_(*this);
        }

        /** Returns the width of the canvas. 
         */
        int width() const {
            return width_;
        }

        /** Returns the height of the canvas. 
         */
        int height() const {
            return height_;
        }

        /** Returns the rectangle of the canvas. 
         */
        Rect rect() const {
            return Rect::FromWH(width_, height_);
        }

        /** Resizes the canvas. 
         
            Returns a new canvas that is identical to the current canvas except for the size which can be specified. 
         */ 
        Canvas resize(int width, int height) const {
            return Canvas{width, height, buffer_, visibleRect_, bufferOffset_};
        }

        /** Offsets the canvas. 
         
            Creates a canvas identical to the current canvas except for the position of the visible rectangle, which will be offset by the given coordinates. This corresponds to the canvas being scrolled as different part of it will actually be visible.
         */
        //@{
        Canvas offset(Point by) const {
            return Canvas{width_, height_, buffer_, (visibleRect_ + by) & rect(), bufferOffset_ - by};
        }

        Canvas offset(int left, int top) const {
            return offset(Point{left, top});
        }
        //@}

        /** Clips the canvas. 
         
            The returned canvas will correspond to the specified rectangle of the current canvas. Note that the rectangle does not have to be fully contained within the current canvas. 

         */
        Canvas clip(Rect const & rect) const {
            return Canvas{
                rect.width(),
                rect.height(),
                buffer_,
                (rect & visibleRect_) - rect.topLeft(),
                bufferOffset_ + rect.topLeft()
            };
        }

        /** Returns the visible rectangle of the canvas, i.e. the part of the canvas that is backed by an underlying buffer. 
         
            Drawing outside the visible rect is permitted, but has no effect. 
         */
        Rect visibleRect() const {
            return visibleRect_;
        }

        // ========================================================================================

        /** \name Painting structures
         */
        //@{

        Color fg() const {
            return state_.fg();
        }

        Canvas & setFg(Color value) {
            ASSERT(value.a == 255);
            state_.setFg(value);
            return *this;
        }

        Color bg() const {
            return state_.bg();
        }

        Canvas & setBg(Color value) {
            ASSERT(value.a == 255);
            state_.setBg(value);
            return *this;
        }

        Color decor() const {
            return state_.decor();
        }

        Canvas & setDecor(Color value) {
            ASSERT(value.a == 255);
            state_.setDecor(value);
            return *this;
        }

        Font font() const {
            return state_.font();
        }

        Canvas & setFont(Font value) {
            state_.setFont(value);
            return *this;
        }

        //@}

        // ========================================================================================

        /** \name Painting functions
         */
        //@{

        Canvas & setAt(Point coords, Cell const & cell) {
            Cell * c = at(coords);
            if (c != nullptr)
                *c = cell;
            return *this;
        }

        Canvas & setBorderAt(Point coords, Border const & border) {
            Cell * c = at(coords);
            if (c != nullptr)
                c->setBorder(border);
            return *this;
        }

        Canvas & drawBuffer(Buffer const & buffer, Point topLeft);
        
        Canvas & fillRect(Rect const & rect);

        /** Fills the given rectangle with specified background color, leaving everything else intact. 
         */
        Canvas & fillRect(Rect const & rect, Color background);

        /** Draws the specified border line. 
         
            The line must be either horizontal, or vertical. The border of all cells along the line will be updated with the provided one. The update allows drawBorderLine to be used to draw even corners of border rectangles. 
         */
        Canvas & drawBorderLine(Border const & border, Point from, Point to);

        /** Applies the border to given rectangle edges.
         
            Sets the border 
         */
        Canvas & drawBorderRect(Border const & border, Rect const & rect);

        /** Draws the specified text specified by utf8 iterators. 
         
            The text is drawn in a single line starting from the specified point and growing to the right. 
         */
        Canvas & textOut(Point where, Char::iterator_utf8 begin, Char::iterator_utf8 end);

        Canvas & textOut(Point where, StringLine const & line, int maxWidth, HorizontalAlign hAlign);

        /** Draws the specified text starting from the given point. 
         
            The text is drawn in a single line starting from the specified point and growing to the right. 
         */
        Canvas & textOut(Point where, std::string const & text) {
            return textOut(where, Char::BeginOf(text), Char::EndOf(text));
        }

        //@}

        /** Adds new finalizer to the canvas. 
         
            If the canvas already has a finalizer, the new finalizer will first call itself and then call the old one, ensuring that first registered finalizer will run last. 
         */
        void addFinalizer(Finalizer value) {
            if (! finalizer_) {
                finalizer_ = value;
            } else {
                Finalizer old = finalizer_;
                finalizer_ = [old, value](Canvas & canvas){
                    value(canvas);
                    old(canvas);
                };
            }
        }

    protected:

        friend class Widget;

        Canvas(int width, int height, Buffer & buffer, Rect visibleRect, Point visibleRectOffset):
            width_{width},
            height_{height},
            buffer_{buffer}, 
            visibleRect_{visibleRect},
            bufferOffset_{visibleRectOffset} {
            // TODO check the validity of the visible rectangle and update accordingly
        }

        /** Returns the cell at given coordinates.
         
            If the coordinates are outside of the visible rectangle, nullptr is returned. 
         */
        //@{
        Cell const * at(Point p) const {
            return const_cast<Canvas*>(this)->at(p);
        }

        Cell * at(Point p) {
            if (! visibleRect_.contains(p))
                return nullptr;
            return & buffer_.at(p + bufferOffset_);
        }
        //@}

        /** Updates border of given cell if the cell is in the visible area. 
         
            Does not change the unused bits. 
         */
        void updateBorder(Point p, Border const & border) {
            if (visibleRect_.contains(p)) {
                // get the cell ourselves bypassing the buffer's unused bits clear
                p += bufferOffset_;
                Cell & c = buffer_.rows_[p.y()][p.x()];
                c.setBorder(c.border().updateWith(border));
            }
        }

    private:

        int width_;
        int height_;

        /** The backing buffer for the canvas. 
         */
        Buffer & buffer_;

        /** \anchor ui_canvas_visible_rect 
            \name Visible Rectangle Properties

            The coordinates of the top-left corner (origin) of the canvas in the buffer's coordinates and the visible rectangle, i.e. the rectangle of the canvas that is visible. The canvas is not allowed to change any cells outside of the visible rectangle. 

            Note that the visible rectangle is in the canvas' own coordinates. 
         */
        /** The visible rectangle of the canvas in the canvas coordinates. 
         */
        Rect visibleRect_;

        /** The coordinates of canvas origin in the backing buffer's coordinates. 
         
            I.e. value that needs to be added to a point in canvas coordinates to convert it to renderer's buffer coordinates.  
         */
        Point bufferOffset_;

        /** The drawing state (foreground, background, font, character etc.)
         */
        Cell state_;

        Finalizer finalizer_;

    };

} // namespace ui
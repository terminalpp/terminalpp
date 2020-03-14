#pragma once

#include "buffer.h"
#include "geometry.h"

namespace ui2 {

    class Widget;

    /** 
     */
    class Canvas {
    public:
        /** Creates a canvas that encompasses the entire buffer. 
         
            Only the main UI thread can create a canvas. 
         */
        Canvas(Widget * widget);

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
                visibleRect_ & rect,
                bufferOffset_ - rect.topLeft()
            };
        }

        // ========================================================================================

        /** \name Painting structures
         */
        //@{

        Color fg() const {
            return fg_;
        }

        Canvas & setFg(Color value) {
            fg_ = value;
            return *this;
        }

        Brush const & bg() const {
            return bg_;
        }

        Canvas & setBg(Brush const & value) {
            bg_ = value;
            return *this;
        }

        Color decor() const {
            return decor_;
        }

        Canvas & setDecor(Color value) {
            decor_ = value;
            return *this;
        }

        Font font() const {
            return font_;
        }

        Canvas & setFont(Font value) {
            font_ = value;
            return *this;
        }

        //@}

        // ========================================================================================

        /** \name Painting functions
         */
        //@{
        
        Canvas & fillRect(Rect const & rect);

        //@}

    protected:

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
            if (! visibleRect_.contains(p))
                return nullptr;
            return & buffer_.at(p + bufferOffset_);
        }

        Cell * at(Point p) {
            return const_cast<Canvas*>(this)->at(p);
        }
        //@}

        void applyBrush(Cell & cell, Brush const & brush) {
            cell.setBg(brush.color()).setFont(brush.font());
            if (brush.fill() != 0) {
                cell.setCodepoint(brush.fill());
                cell.setFg((brush.fillColor() != Color::None) ? brush.fillColor() : fg_);
                cell.setDecor((brush.fillColor() != Color::None) ? brush.fillColor() : fg_);
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




        Color fg_;
        Brush bg_;
        Color decor_;
        Font font_;

    };


} // namespace ui2
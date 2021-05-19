#pragma once

#include "helpers/tests.h"

#include "geometry.h"

namespace ui {

    /** Single character cell of the ui's buffer. 
     */
    class Cell {

    }; // ui::Cell

    /** UI backing buffer, a 2D array of cells. 
     */
    class Buffer {
    public:
        Cell const & operator [] (Point at) const {

        }

        Cell & operator [] (Point at) {

        }


    }; // ui::Buffer

    /** Selection of basic drawing tools into a locked buffer. 
     */
    class Canvas {
        friend class Widget;
    public:

    protected:
        class VisibleRect {
        public:

            VisibleRect() = default;

            VisibleRect(Rect rect, Point offset):
                rect{rect},
                offset{offset} {
            }

            bool empty() const {
                return rect.empty();
            }

            Point toBuffer(Point local) const {
                return local + offset;
            }

            Point toLocal(Point buffer) const {
                return buffer - offset;
            }

            /** Offsets the visible rectangle by given vector. 
             
                The offset visible rectangle is of the same size and buffer coordinates, but with local rectangle offset by the specified point. 
             */
            VisibleRect offsetBy(Point local) const {
                return VisibleRect{
                    rect + local, 
                    offset - local
                };
            }

            /** Creates a new visible rectangle by clipping the current one. 
             
                The clip is given in local coordinates. If the clipped rectangle and the actual visible rectangle have no intersection, returns an empty visible rectangle. 
             */
            VisibleRect clip(Rect localClip) const {
                return VisibleRect{
                    (rect & localClip) - localClip.topLeft(),
                    offset + localClip.topLeft()
                };
            }
            
            /** Visible rectangle of the canvas, in canvas coordinates. 
             */
            Rect rect;

            /** The offset of the canvas compared to the actual buffer.
             
                I.e. the coordinates of the [0,0] canvas point in the canvas' backing buffer, irrespective of whether the origin actually lies in the visible rectangle.

                To convert canvas coordinates to buffer coordinates, the offset has simply to be added. This is of course only meaningful if the canvas coordinates fall within the visible rectangle itself. 
             */
            Point offset;
        };


    private:

        Cell * at(Point p) {
            if (! visibleRect_.rect.contains(p))
                return nullptr;
            else
                return & buffer_[p + visibleRect_.offset];    
        }

        Buffer & buffer_; // backing buffer of the canvas
        Size size_; // the size of the canvas
        VisibleRect visibleRect_; // visible part of the canvas


    private:
        FRIEND_TEST(VisibleRect, CoordTransformations);
        FRIEND_TEST(VisibleRect, Offset);
        FRIEND_TEST(VisibleRect, Clip);
    }; // ui::Canvas


} // namespace ui
#pragma once

#include "geometry.h"

namespace ui3 {

    class Renderer;

    class Canvas {
        friend class Widget;
        friend class Renderer;
    public:

    protected:
        

        /** Visible area of the canvas. 
         
            Each widget remembers its visible area, which consists of the pointer to its renderer, the offset of the widget's top-left corner in the renderer's absolute coordinates and the area of the widget that translates to a portion of the renderer's buffer. 
        */
        class VisibleArea {
            friend class Renderer;
        public:
            Renderer * renderer() const {
                return renderer_;
            }

            Point offset() const {
                return offset_;
            }

            Rect const & rect() const {
                return rect_;
            }

            /** The visible are in buffer coordinates. 
             */
            Rect bufferRect() const {
                return rect_ + offset_;
            }

            bool attached() const {
                return renderer_ != nullptr;
            }

            /** Detaches the visible area from the renderer, thus invalidating it. 
             */
            void detach() {
                renderer_ = nullptr;
            }

            VisibleArea clip(Rect const & rect) const {
                return VisibleArea(renderer_, offset_ + rect.topLeft(), (rect_ & rect) - rect.topLeft());
            }

            VisibleArea offset(Point const & by) const {
                NOT_IMPLEMENTED;
            }

        private:

            VisibleArea(Renderer * renderer, Point const & offset, Rect const & rect):
                renderer_{renderer}, 
                offset_{offset},
                rect_{rect} {
            }

            Renderer * renderer_;
            Point offset_;
            Rect rect_;
        }; // ui::Canvas::VisibleArea

        Canvas(VisibleArea const & visibleArea):
            visibleArea_{visibleArea} {
        }

    private:

        VisibleArea visibleArea_;

    }; // ui::Canvas

} // namespace ui
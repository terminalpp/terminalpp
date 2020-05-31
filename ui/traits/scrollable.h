#pragma once

#include "helpers/time.h"

#include "../widget.h"

#include "trait_base.h"

namespace ui {

    /** Scrollable widgets trait. 
     
        Implements the basic data and functionality for scrolling. 

        - scrolling does not really support borders around the the scrollthingy, i.e. scrollbox must have no borders

     */
	template<typename T>
	class Scrollable : public TraitBase<Scrollable, T> {
	public:

        virtual Point scrollOffset() const {
            return scrollOffset_;
        }

        virtual Size scrollSize() const {
            return scrollSize_;
        }

    protected:

        using TraitBase<Scrollable, T>::downcastThis;

        Scrollable(int width, int height):
            scrollSize_{width, height},
            scrollOffset_{0,0} {
        } 

        virtual void setScrollOffset(Point offset) {
            if (scrollOffset_ != offset) {
                scrollOffset_ = offset;
                downcastThis()->repaint();
            }
        }

        /** Incremental scrolling
         */
        bool scrollBy(Point by) {
            Point offset = scrollOffset_ + by;
            Point adjusted = Point::MinCoordWise(
                Point::MaxCoordWise(Point{0,0}, offset), 
                Point{scrollSize_.width() - downcastThis()->width(), scrollSize_.height() - downcastThis()->height()}
            );
            setScrollOffset(adjusted);
            return adjusted == offset;
        }

        virtual void setScrollSize(Size value) {
            if (scrollSize_ != value) {
                // TODO should we check aor repaint anything? 
                scrollSize_ = value;
            }
        }

        /** Shorthand for getting the canvas of the scrollable content. 
         
            The canvas is calculated from the widget's canvas by first resizing it to the scroll size and then offseting the visible rectangle by the scroll offset.
         */
        Canvas getContentsCanvas(Canvas & widgetCanvas) {
            return widgetCanvas.resize(scrollSize_).offset(scrollOffset_);
        }

        /** Displays the scrollbars. 
         
            Scrollbars are displayed only when the canvas size is smaller than the scrollWidth and height. 
         */
        void paint(Canvas & canvas) {
            if (scrollSize_.height() > canvas.height()) {
                std::pair<int, int> slider{ScrollBarDimensions(canvas.height(), scrollSize_.height(), scrollOffset_.y())};
                paintHorizontalScrollbar(canvas, slider.first, slider.second);
            }
            if (scrollSize_.width() > canvas.width()) {
                std::pair<int, int> slider{ScrollBarDimensions(canvas.width(), scrollSize_.width(), scrollOffset_.x())};
                paintVerticalScrollbar(canvas, slider.first, slider.second);
            }
        }

        virtual void paintHorizontalScrollbar(Canvas & canvas, int start, int end) {
            Border b{Color::White.withAlpha(64)};
            b.setRight(Border::Kind::Thin);
            int x = canvas.width() - 1;
            canvas.drawBorderLine(b, Point{x, 0}, Point{x, start});
            canvas.drawBorderLine(b, Point{x, end}, Point{x, canvas.height()});
            b.setRight(Border::Kind::Thick);
            canvas.drawBorderLine(b, Point{x, start}, Point{x, end});
        }

        virtual void paintVerticalScrollbar(Canvas & canvas, int start, int end) {
            Border b{Color::White.withAlpha(64)};
            b.setRight(Border::Kind::Thin);
            int y = canvas.height() - 1;
            canvas.drawBorderLine(b, Point{0, y}, Point{start, y});
            canvas.drawBorderLine(b, Point{end, y}, Point{canvas.height(), y});
            b.setRight(Border::Kind::Thick);
            canvas.drawBorderLine(b, Point{start, y}, Point{end, y});
        }

    private:
        Size scrollSize_;
        Point scrollOffset_;

        static std::pair<int, int> ScrollBarDimensions(int length, int max, int offset) {
            int sliderSize = std::max(1, length * length / max);
            int sliderStart = (offset + length == max) ? (length - sliderSize) : (offset * length / max);
			// make sure that slider starts at the top only if we are really at the top
			if (sliderStart == 0 && offset != 0)
			    sliderStart = 1;
            // if the slider would go beyond the length, adjust the slider start
            if (sliderStart + sliderSize > length)
                sliderStart = length - sliderSize;
            return std::make_pair(sliderStart, sliderStart + sliderSize);
        }	


    }; // ui::Scrollable


    /** Autoscrolling trait
     
        The autoscrolling trait provides a timer and increment that can be used to auto scroll widgets when needed. The AutoScroller does not implement the actual scrolling so that it can be inherited by any Widget. This is useful when a non-scrollable widget controls a scrollable widget and therefore has to provide this forwarding.

        If the widget to be autoscrolled is the widget itself, then OwnAutoScroller should be used. 
     */
    template<typename T>
    class AutoScroller : public TraitBase<AutoScroller, T> {
    protected:
        using TraitBase<AutoScroller, T>::downcastThis;

        /** Creates the autoscroller.
         */
        AutoScroller():
            autoScrollIncrement_{0,0} {
            autoScrollTimer_.setInterval(50);
            autoScrollTimer_.setHandler([this]() {
                return autoScrollStep(autoScrollIncrement_);
            });
        }

        virtual ~AutoScroller() { }

        /** Starts the autoscroll feature. 
         
            Each step, the scroll offset will be updated by the given step util it is either stopped, or reaches the scrolling limits. 
         */
        void startAutoScroll(Point const & step) {
            autoScrollTimer_.stop();
            autoScrollIncrement_ = step;
            autoScrollTimer_.start();
        }

        /** Stops the autoscroll feature if active. 
          
            Does nothing if the the autoscroll is already stopped. 
         */
        void stopAutoScroll() {
            autoScrollTimer_.stop();
        }

        /** Returns true if the autoscroll feature is currently active. 
         */
        bool autoScrollActive() const {
            return autoScrollTimer_.running();
        }

        /** A single step of the autoscroll feature. 
         
            Should perform the scrolling and return true if more scrolling in the desired direction is possible, false otherwise. When false is returned, the auto scrolling is stopped. 
         */    
        virtual bool autoScrollStep(Point by) = 0;

    private:
        Point autoScrollIncrement_;

        Timer autoScrollTimer_;

    }; // ui::AutoScroller

    /** AutoScroller specialization that scrolls its own contents.
     
        This class simply provides the autoScrollStep() method implementation so that the widget scrolls itself. 
     */
    template<typename T>
    class OwnAutoScroller : public AutoScroller<T> {
    protected:
        using AutoScroller<T>::downcastThis;

        /** Scrolls the widget itself. 
         */
        bool autoScrollStep(Point by) override {
            T* self = downcastThis();
            return self->scrollBy(by);
        }
    };

} // namespace ui


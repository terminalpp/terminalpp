
#include <utility>
#include "../widget.h"
#include "../layout.h"
#include "../geometry.h"

namespace ui {

    class ScrollBar : public Widget {
    public:
        /** ScrollBar's position. 
         
            The left and right scrollbars are always vertical, while the top and bottom scrollbars are horizontal, if target widget is used. 
         */
        enum class Position {
            Left,
            Right,
            Top,
            Bottom,
        }; // ui::ScrollBar::Position

        ScrollBar() = default;

        ScrollBar(Position pos): 
            position_{pos} {
        }

        Position position() const {
            return position_;
        }

        virtual void setPosition(Position value) {
            if (position_ != value) {
                position_ = value;
                requestRepaint();
            }
        }

        int min() const {
            return min_;
        }

        int max() const {
            return max_;
        }

        virtual void setMax(int value) {
            if (max_ != value) {
                max_ = value;
                if (value_ > max_)
                    value_ = max_;
                requestRepaint();
            }
        }

        int value() const {
            return value_;
        }

        virtual void setValue(int value) {
            if (value_ < min_)
                value_ = min_;
            else if (value_ > max_)
                value_ = max_;
            if (value_ != value) {
                value_ = value;
                requestRepaint();
            }
        }

        int sliderSize() const {
            return sliderSize_;
        }

        virtual void setSliderSize(int value) {
            if (sliderSize_ != value) {
                sliderSize_ = value;
                requestRepaint();
            }
        }

    protected:

        /** Paints the scrollbar.
         */
        void paint(Canvas & canvas) override {
            switch (position_) {
                case Position::Left: {
                    Border b = Border::Empty(color_).setLeft(Border::Kind::Thin);
                    Border slider = Border::Empty(color_).setLeft(Border::Kind::Thick);
                    paintVertical(canvas, 0, b, slider);
                    break;
                }
                case Position::Right: {
                    Border b = Border::Empty(color_).setRight(Border::Kind::Thin);
                    Border slider = Border::Empty(color_).setRight(Border::Kind::Thick);
                    paintVertical(canvas, width() - 1, b, slider);
                    break;
                }
                case Position::Top: {
                    Border b = Border::Empty(color_).setTop(Border::Kind::Thin);
                    Border slider = Border::Empty(color_).setTop(Border::Kind::Thick);
                    paintHorizontal(canvas, 0, b, slider);
                    break;
                }
                case Position::Bottom: {
                    Border b = Border::Empty(color_).setBottom(Border::Kind::Thin);
                    Border slider = Border::Empty(color_).setBottom(Border::Kind::Thick);
                    paintHorizontal(canvas, height() - 1, b, slider);
                    break;
                }
            }
        }

        void paintHorizontal(Canvas & canvas, int row, Border & b, Border & slider) {
            std::pair<int, int> dim = sliderDimensions(width());
            canvas.setBorder(Point{0,row}, Point{dim.first, row}, b);
            canvas.setBorder(Point{dim.second, row}, Point{width(), row}, b);
            canvas.setBorder(Point{dim.first, row}, Point{dim.second, row}, slider);
        }

        void paintVertical(Canvas & canvas, int col, Border & b, Border & slider) {
            std::pair<int, int> dim = sliderDimensions(height());
            canvas.setBorder(Point{col, 0}, Point{col, dim.first}, b);
            canvas.setBorder(Point{col, dim.second}, Point{col, height()}, b);
            canvas.setBorder(Point{col, dim.first}, Point{col, dim.second}, slider);
        }

        /** Determines the size of the slider based on the scrollbar attributes and its size given as argument. 
         
            Returns the start (inclusive) and end (exclusive) of the slider for a scrollbar of given size. 
         */
        std::pair<int, int> sliderDimensions(int scrollBarSize) {
            // adjust the value and size so that they appear to start at 0
            int adjustedSize = max_ - min_;
            int adjustedValue = value_ - min_;
            // calculate slider size in cells, which must be at least one
            int sliderSize = std::max(1, sliderSize_ * scrollBarSize / adjustedSize);
            // calculate slider start and make sure the slider start and slider fall within the slider itself and that the slider is not at the top or bottom position unless value is in the extreme
            int sliderStart = adjustedValue * scrollBarSize / adjustedSize;
            if (sliderStart == 0 && adjustedValue != 0)
                sliderStart = 1;
            if (sliderStart + sliderSize > scrollBarSize)
                sliderStart = scrollBarSize - sliderSize;
            // and return 
            return std::make_pair(sliderStart, sliderStart + sliderSize);
        }	
        
    private:

        /** Position of the scrollbar's slider. */
        Position position_;
        /** Color of the slider and scrollbar. */
        Color color_ = Color::White.withAlpha(64);
        /** Minimal value of the scrolled value. */
        int min_ = 0;
        /** Maximal value of the scrolled value. */
        int max_ = 100;
        /** Current value of the scrollbar. */
        int value_ = 0;
        /** The size of the slider (in the min-max range). */
        int sliderSize_ = 1;

    }; // ui::Scrollbar
 

    /** Scrollable widget. 
     
     */
    class ScrollBox : public virtual Widget {
    public:

        ScrollBox():
            horizontal_{new ScrollBar{}},
            vertical_{new ScrollBar{}} {
            horizontal_->setPosition(ScrollBar::Position::Right);
            vertical_->setPosition(ScrollBar::Position::Bottom);
            attach(horizontal_);
            attach(vertical_);
            setLayout(new Layout::Maximized{});    
        }

        /** Returns the horizontal scroolbar. 
         */
        ScrollBar * scrollBarHorizontal() const {
            return horizontal_;
        }

        /** Returns the vertical scrollbar. 
         */
        ScrollBar * scrollBarVertical() const {
            return vertical_;
        }

        /** Returns the scrolled contents widget. 
         */
        Widget * contents() const {
            return children().size() == 2 ? nullptr : children()[0];
        }

        /** Sets the scrolled widget. 
         
            The widget is immediate child of the scrollbox. 
         */
        virtual void setContents(Widget * contents) {
            if (children().size() == 3 && children()[0] != contents)
                detach(children()[0]);
            if (contents != nullptr)
                attachBack(contents);
        }

    protected:

        /** Returns the contents canvas. 
         
            The contents canvas is created by offseting the widget's contents canvas by the scrollOffset(). 
         */
        Canvas getContentsCanvas(Canvas & widgetCanvas) const override {
            return Widget::getContentsCanvas(widgetCanvas).offsetBy(scrollOffset_);
        }

    private:

        ScrollBar * vertical_;
        ScrollBar * horizontal_;


        Point scrollOffset_;

    }; // ui::ScrollBox

} // namespace ui
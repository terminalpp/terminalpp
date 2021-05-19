#pragma once

#include "helpers/helpers.h"

namespace ui {

    enum class HorizontalAlign {
        Left,
        Center,
        Right
    }; // ui::HorizontalAlign

    enum class VerticalAlign {
        Top,
        Middle,
        Bottom
    }; // ui::VerticalAlign

    class Point {
    public:

        Point(): 
            x_{0},
            y_{0} {
        }

        Point(int x, int y):
            x_{x},
            y_{y} {
        }

        int x() const {
            return x_;
        }

        void setX(int value) {
            x_ = value;
        }

        int y() const {
            return y_;
        }

        void setY(int value) {
            y_ = value;
        }

        Point & operator = (Point const & other) = default;

        Point & operator += (Point const & other) {
            x_ += other.x_;
            y_ += other.y_;
            return *this;
        }

        Point & operator -= (Point const & other) {
            x_ -= other.x_;
            y_ -= other.y_;
            return *this;
        }

        Point operator * (int by) const {
            return Point{x_ * by, y_ * by};
        }

        Point operator / (int by) const {
            return Point{x_ / by, y_ / by};
        }

        Point operator * (double by) const {
            return Point{static_cast<int>(x_ * by), static_cast<int>(y_ * by)};
        }

        Point operator / (double by) const {
            return Point{static_cast<int>(x_ / by), static_cast<int>(y_ / by)};
        }

        Point operator + (Point const & other) const {
            return Point{x_ + other.x_, y_ + other.y_};
        }

        Point operator - (Point const & other) const {
            return Point{x_ - other.x_, y_ - other.y_};
        }

        bool operator == (Point const & other) const {
            return x_ == other.x_ && y_ == other.y_;
        }

        bool operator != (Point const & other) const {
            return x_ != other.x_ || y_ != other.y_;
        }

        bool operator >= (Point const & other) const {
            return x_ >= other.x_ && y_ >= other.y_;
        }

        bool operator > (Point const & other) const {
            return x_ > other.x_ && y_ > other.y_;
        }

        bool operator <= (Point const & other) const {
            return x_ <= other.x_ && y_ <= other.y_;
        }

        bool operator < (Point const & other) const {
            return x_ < other.x_ && y_ < other.y_;
        }

    private:
        friend class Rect;

        int x_;
        int y_;

        friend std::ostream & operator << (std::ostream & s, Point const & p) {
            s << "[" << p.x_ << ", " << p.y_ << "]";
            return s;
        }

    }; 

    class Size {
    public:

        Size():
            width_{0},
            height_{0} {
        }

        Size(int width, int height):
            width_{width},
            height_{height} {
        }

        bool empty() const {
            return width_ == 0 || height_ == 0;
        }

        int width() const {
            return width_;
        }

        void setWidth(int value) {
            width_ = value;
        }

        int height() const {
            return height_;
        }

        void setHeight(int value) {
            height_ = value;
        }

        bool operator == (Size const & other) const {
            return width_ == other.width_ && height_ == other.height_;
        }

        bool operator != (Size const & other) const {
            return width_ != other.width_ || height_ != other.height_;
        }

        friend Point operator + (Point const & p, Size const & s) {
            return Point{p.x() + s.width(), p.y() + s.height()};
        }

        friend Size operator * (Size const & size, double by) {
            return Size{static_cast<int>(size.width_ * by), static_cast<int>(size.height_ * by)};
        }

        friend Size operator / (Size const & size, double by) {
            return Size{static_cast<int>(size.width_ / by), static_cast<int>(size.height_ / by)};
        }

    private:
        friend class Rect;

        int width_;
        int height_;
    }; 

    class Rect {
    public:

        /** Creates an empty rect. 
         */
        Rect():
            topLeft_{0,0},
            size_{0,0} {
        }

        /** Creates an empty rectangle. 
         
            This is identical to the default constructor, but looks better.
         */
        static Rect Empty() {
            return Rect{};
        }

        explicit Rect(Size const & size):
            topLeft_{0,0},
            size_{size} {
        }

        Rect(Point const & topLeft, Size const & size):
            topLeft_{topLeft},
            size_{size} {
        }

        Rect(Point topLeft, Point bottomRight):
            Rect{topLeft, bottomRight, false} {
        }

        Rect(Point topLeft, Point bottomRight, bool noSwap) {
            if (noSwap) {
                topLeft_ = topLeft;
                if (topLeft.x() <= bottomRight.x() && topLeft.y() <= bottomRight.y())
                    size_ = Size{bottomRight.x() - topLeft.x(), bottomRight.y() - topLeft.y()};
            } else {
                if (topLeft.x() > bottomRight.x())
                    std::swap(topLeft.x_, bottomRight.x_);
                if (topLeft.y() > bottomRight.y())
                    std::swap(topLeft.y_, bottomRight.y_);
                topLeft_ = topLeft;
                size_ = Size{bottomRight.x() - topLeft.x(), bottomRight.y() - topLeft.y()};
            }
        }

        bool empty() const {
            return size_.empty();
        }

        Point topLeft() const {
            return topLeft_;
        }

        Point topRight() const {
            return topLeft_ + Point{size_.width(), 0};
        }

        Point bottomLeft() const {
            return topLeft_ + Point{0, size_.height()};
        }

        Point bottomRight() const {
            return topLeft_ + size_;
        }

        int top() const {
            return topLeft_.y();
        }

        int left() const {
            return topLeft_.x();
        }

        int bottom() const {
            return topLeft_.y() + size_.height();
        }

        int right() const {
            return topLeft_.x() + size_.width();
        }

        Size size() const {
            return size_;
        }

        int width() const {
            return size_.width();
        }

        int height() const {
            return size_.height();
        }

        void move(Point const & topLeft) {
            topLeft_ = topLeft;
        }

        void resize(Size const & size) {
            size_ = size;
        }

        bool contains(Point const & p) const {
            return p >= topLeft_ && p < bottomRight();
        }

        Point align(Rect const & rect, HorizontalAlign hAlign, VerticalAlign vAlign) const {
            return Point{
                align(rect.width(), hAlign),
                align(rect.height(), vAlign)
            };
        }
        Point align(Rect const & rect, HorizontalAlign hAlign) const {
            return Point{
                align(rect.width(), hAlign),
                rect.top()
            };
        }
        Point align(Rect const & rect, VerticalAlign vAlign) const {
            return Point {
                rect.left(),
                align(rect.height(), vAlign)
            };
        }


        Rect operator + (Point const & p) const {
            return Rect{topLeft_ + p, size_};
        }

        Rect operator - (Point const & p) const {
            return Rect{topLeft_ - p, size_};
        }

        /** Intersection of two rectangles. 
         */
        Rect operator & (Rect const & other) const {
            return Rect(
                Point{
                    std::max(topLeft_.x(), other.topLeft_.x()),
                    std::max(topLeft_.y(), other.topLeft_.y())
                },
                Point{
                    std::min(bottomRight().x(), other.bottomRight().x()),
                    std::min(bottomRight().y(), other.bottomRight().y())
                },
                /* noSwap */ true);
        }

        /** Union of two rectangles. 
         */
        Rect operator | (Rect const & other) const {
            return Rect(
                Point{
                    std::min(topLeft_.x(), other.topLeft_.x()),
                    std::min(topLeft_.y(), other.topLeft_.y())
                },
                Point{
                    std::max(bottomRight().x(), other.bottomRight().x()),
                    std::max(bottomRight().y(), other.bottomRight().y())
                },
                /* noSwap */ true);
        }

    private:

        int align(int childWidth, HorizontalAlign align) const {
            switch (align) {
                default:
                case HorizontalAlign::Left:
                    return left();
                case HorizontalAlign::Center:
                    return left() + (width() - childWidth) / 2;
                case HorizontalAlign::Right:
                    return right() - childWidth;
            }
        }

        int align(int childHeight, VerticalAlign align) const {
            switch (align) {
                default:
                case VerticalAlign::Top:
                    return top();
                case VerticalAlign::Middle:
                    return top() + (height() - childHeight) / 2;
                case VerticalAlign::Bottom:
                    return bottom() - childHeight;
            }
        }


        Point topLeft_;
        Size size_;
    };    


} // namespace ui
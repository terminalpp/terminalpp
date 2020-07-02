#pragma once

#include <algorithm>

#include "helpers/helpers.h"

namespace ui3 {

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

        int y() const {
            return y_;
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

        int height() const {
            return height_;
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

    private:
        friend class Rect;

        int width_;
        int height_;
    }; 

    class Rect {
    public:

        Rect():
            topLeft_{0,0},
            size_{0,0} {
        }

        Rect(Size const & size):
            topLeft_{0,0},
            size_{size} {
        }

        Rect(Point const & topLeft, Size const & size):
            topLeft_{topLeft},
            size_{size} {
        }

        Rect(Point topLeft, Point bottomRight) {
            if (topLeft.x() > bottomRight.x())
                std::swap(topLeft.x_, bottomRight.x_);
            if (topLeft.y() > bottomRight.y())
                std::swap(topLeft.y_, bottomRight.y_);
            topLeft_ = topLeft;
            size_ = Size{bottomRight.x() - topLeft.x(), bottomRight.y() - topLeft.y()};
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

        Size size() const {
            return size_;
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
                });
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
                });
        }

    private:

        Point topLeft_;
        Size size_;
    };


} // namespace ui
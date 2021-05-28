#pragma once

#include "helpers/helpers.h"
#include "helpers/char.h"

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

    class Point final {
    public:

        Point() = default;

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

        int x_ = 0;
        int y_ = 0;

        friend std::ostream & operator << (std::ostream & s, Point const & p) {
            s << "[" << p.x_ << ", " << p.y_ << "]";
            return s;
        }

    }; 

    class Size final {
    public:

        Size() = default;

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

        int width_ = 0;
        int height_ = 0;
    }; 

    class Rect final {
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

        static Rect CreateOrEmpty(Point topLeft, Point bottomRight) {
            if (topLeft.x() <= bottomRight.x() && topLeft.y() <= bottomRight.y())
                return Rect{topLeft, Size{bottomRight.x() - topLeft.x(), bottomRight.y() - topLeft.y()}};
            else
                return Empty();
        }

        explicit Rect(Size const & size):
            topLeft_{0,0},
            size_{size} {
        }

        Rect(Point const & topLeft, Size const & size):
            topLeft_{topLeft},
            size_{size} {
        }

        Rect(Point topLeft, Point bottomRight) {
            //Rect{topLeft, bottomRight, false} {
            if (topLeft.x() > bottomRight.x())
                std::swap(topLeft.x_, bottomRight.x_);
            if (topLeft.y() > bottomRight.y())
                std::swap(topLeft.y_, bottomRight.y_);
            topLeft_ = topLeft;
            size_ = Size{bottomRight.x() - topLeft.x(), bottomRight.y() - topLeft.y()};
        }
/*
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
    */

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
            return CreateOrEmpty(
                Point{
                    std::max(topLeft_.x(), other.topLeft_.x()),
                    std::max(topLeft_.y(), other.topLeft_.y())
                },
                Point{
                    std::min(bottomRight().x(), other.bottomRight().x()),
                    std::min(bottomRight().y(), other.bottomRight().y())
                }
            );
        }

        /** Union of two rectangles. 
         */
        Rect operator | (Rect const & other) const {
            return CreateOrEmpty(
                Point{
                    std::min(topLeft_.x(), other.topLeft_.x()),
                    std::min(topLeft_.y(), other.topLeft_.y())
                },
                Point{
                    std::max(bottomRight().x(), other.bottomRight().x()),
                    std::max(bottomRight().y(), other.bottomRight().y())
                }
            );
        }

        bool operator == (Rect const & other) const {
            return topLeft_ == other.topLeft_ && size_ == other.size_;
        }

        bool operator != (Rect const & other) const {
            return topLeft_ != other.topLeft_ || size_ != other.size_;
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


    class Color final {
    public:
        uint8_t a;
        uint8_t b;
        uint8_t g;
        uint8_t r;

        Color():
            Color{0,0,0,0} {
        }

        static Color RGB(uint8_t r, uint8_t g, uint8_t b) {
            return Color{r, g, b, 255};
        }

        static Color RGBA(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
            return Color{r, g, b , a};
        }

        static Color HTML(std::string const & colorCode) {
			unsigned start = colorCode[0] == '#' ? 1 : 0;
			if (colorCode.size() - start < 6)
			    THROW(IOError()) << "Exepected at least RRGGBB color definition but " << colorCode << " found.";
			unsigned char r = static_cast<unsigned char>(ParseHexNumber(colorCode.c_str() + start, 2));
			unsigned char g = static_cast<unsigned char>(ParseHexNumber(colorCode.c_str() + start + 2, 2));
			unsigned char b = static_cast<unsigned char>(ParseHexNumber(colorCode.c_str() + start + 4, 2));
			unsigned char a = 0xff;
			if (colorCode.size() - start == 8)
				a = static_cast<unsigned char>(ParseHexNumber(colorCode.c_str() + start + 6, 2));
			else if (colorCode.size() - start != 6)
			    THROW(IOError()) << "Exepected at least RRGGBBAA color definition but " << colorCode << " found.";
			return RGBA(r, g, b ,a);
        }

        uint32_t toRGB() const {
            return (r << 16) + (g << 8) + b;
        }

        uint32_t toRGBA() const {
            return * pointer_cast<uint32_t const*>(this);
        }

        float floatAlpha() const {
            return static_cast<float>(a) / 255.0f;
        }

        bool opaque() const {
            return a == 255;
        }

        /** Returns a color identical to the current one but with updated alpha value. 
         */
		Color withAlpha(uint8_t value) const {
			return Color{r, g, b, value};
		}

        /** Returns a color obtained by blending the overlay color over the current one. 
         
            Expects the current color to be opaque. If the overlay color is transparent, new opaque color will be generated, if the overlay is opaque, returns simply the overlay color. 
         */
        Color overlayWith(Color overlay) const {
            // overlaying an opaque color just returns the overlay color
            if (overlay.opaque())
                return overlay;
            // if the overlay color is fully transparent, return itself
            if (overlay.a == 0)
                return *this;
            // otherwise we need to calculate the result color
            uint8_t aa = overlay.a + 1;
            uint8_t aInv = static_cast<uint8_t>(256 - overlay.a);
            uint8_t rr = static_cast<uint8_t>((aa * overlay.r + aInv * r) / 256);
            uint8_t gg = static_cast<uint8_t>((aa * overlay.g + aInv * g) / 256);
            uint8_t bb = static_cast<uint8_t>((aa * overlay.b + aInv * b) / 256);
            // this is probably only correct when the other color is opaque,  
            return Color::RGB(rr, gg, bb);
        }

		bool operator == (Color const & other) const {
			return r == other.r && g == other.g && b == other.b && a == other.a;
		}

		bool operator != (Color const & other) const {
			return r != other.r || g != other.g || b != other.b || a != other.a;
		}

        static Color const None;

    private:
        Color(uint8_t r, uint8_t g, uint8_t b, uint8_t a):
            a{a},
            b{b},
            g{g},
            r{r} {
        }
    }; // ui::Font::Color

	inline std::ostream & operator << (std::ostream & s, Color const & c) {
		s << static_cast<unsigned>(c.r) << ";" << static_cast<unsigned>(c.g) << ";" << static_cast<unsigned>(c.b) << ";" << static_cast<unsigned>(c.a);
		return s;
	}

    class Font final {
    public:
        enum class Attribute : uint16_t {
            Bold = (1 << 15),
            Italic = (1 << 14),
            Underline = (1 << 13),
            Strikethrough = (1 << 12),
            Blink = (1 << 11),
            DoubleWidth = (1 << 10),
            DashedUnderline = (1 << 9),
            CurlyUnderline = (1 << 8),
        }; // ui::Font::Attribute

        Font() = default;

        int size() const {
            return (raw_ & 7) + 1;
        }

        Font withSize(int value) {
            ASSERT(value > 0 && value <= 8);
            return Font{static_cast<uint16_t>((raw_ & ~7) + (value - 1))};
        }

        Font & setSize(int value) {
            ASSERT(value > 0 && value <= 8);
            raw_ = (raw_ & ~7) + static_cast<uint16_t>(value - 1);
            return *this;
        }

        Font operator + (Attribute attr) const {
            uint16_t x = raw_ | static_cast<uint16_t>(attr);
            return Font{x};
        } 

        Font & operator += (Attribute attr) {
            raw_ |= static_cast<uint16_t>(attr);
            return *this;
        }

        Font operator - (Attribute attr) const {
            uint16_t x = raw_ & ~(static_cast<uint16_t>(attr)); 
            return Font{x};
        }

        Font & operator -= (Attribute attr) {
            raw_ &= ~(static_cast<uint16_t>(attr));
            return *this;
        }

        bool operator & (Attribute attr) const {
            return raw_ & static_cast<uint16_t>(attr);
        }

        bool operator == (Font const & other) const {
            return raw_ == other.raw_;
        }

        bool operator != (Font const & other) const {
            return raw_ == other.raw_;
        }

        static constexpr Attribute Bold = Attribute::Bold;
        static constexpr Attribute Italic = Attribute::Italic;
        static constexpr Attribute Underline = Attribute::Underline;
        static constexpr Attribute Strikethrough = Attribute::Strikethrough;
        static constexpr Attribute Blink = Attribute::Blink;
        static constexpr Attribute DoubleWidth = Attribute::DoubleWidth;
        static constexpr Attribute DashedUnderline = Attribute::DashedUnderline;
        static constexpr Attribute CurlyUnderline = Attribute::CurlyUnderline;
    private:

        Font(uint16_t raw): 
            raw_{raw} {
        }

        uint16_t raw_ = 0;

    }; // ui::Font

    class Border final {
    public:
        enum class Kind : uint8_t {
            None = 0,
            Thin = 1,
            Thick = 2
        }; // ui::Border::Kind

        enum class Edge : uint8_t {
            Top = 0,
            Left = 2,
            Right = 4,
            Bottom = 6
        }; // ui::Border::Edge

        struct Partial final {
            friend class Border;
        private:
            Partial(Kind kind, Edge edge):
                kind{kind},
                edge{edge} {
            }

            uint8_t addTo(uint8_t raw) const {
                return removeFrom(raw) | (static_cast<uint8_t>(kind) << static_cast<uint8_t>(edge));
            }

            uint8_t removeFrom(uint8_t raw) const {
                return raw & ~(3 << static_cast<uint8_t>(edge));
            }

            Kind kind;
            Edge edge;
        }; // ui::Border::Partial

        static Kind constexpr None = Kind::None;
        static Kind constexpr Thin = Kind::Thin;
        static Kind constexpr Thick = Kind::Thick;

        Border() = default;

        static Border Empty(Color color = Color::None) {
            return Border{color, 0};
        }

        static Border All(Color color, Kind kind = Kind::Thin) {
            uint8_t x = static_cast<uint8_t>(kind);
            return Border{color, static_cast<uint8_t>(x + (x << 2) + (x << 4) + (x << 6))};
        }

        static Partial Top(Kind kind = Kind::Thin) {
            return Partial{kind, Edge::Top};
        }

        static Partial Left(Kind kind = Kind::Thin) {
            return Partial{kind, Edge::Left};
        }

        static Partial Right(Kind kind = Kind::Thin) {
            return Partial{kind, Edge::Right};
        }

        static Partial Bottom(Kind kind = Kind::Thin) {
            return Partial{kind, Edge::Bottom};
        }
        
        bool empty() const {
            return border_ == 0;
        }

        Color const & color() const {
            return color_;
        }

        Border & setColor(Color const & color) {
            color_ = color;
            return *this;
        }

        Border operator + (Partial p) const {
            return Border{color_, p.addTo(border_)};
        }

        Border & operator += (Partial p) {
            border_ = p.addTo(border_);
            return *this;
        }

        Border operator - (Partial p) const {
            return Border{color_, p.removeFrom(border_)};
        }

        Border & operator -= (Partial p) {
            border_ = p.removeFrom(border_);
            return *this;
        }

        bool operator == (Border const & other) const {
            return color_ == other.color_ && border_ == other.border_;
        }

        bool operator != (Border const & other) const {
            return color_ != other.color_ || border_ != other.border_;
        }

    private:

        Border(Color const & color, uint8_t border):
            color_{color},
            border_{border} {
        }

        Color color_;
        uint8_t border_ = 0;


    }; // ui::Border



} // namespace ui
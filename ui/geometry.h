#pragma once

#include <cstdint>
#include <algorithm>

#include "helpers/helpers.h"
#include "helpers/string.h"
#include "helpers/bits.h"

namespace ui {

    class Point {
    public:

        Point(int x = 0, int y = 0):
            x_{x}, 
            y_{y} {
        }

        int x() const {
            return x_;
        }

        int y() const {
            return y_;
        }

        void setX(int value) {
            x_ = value;
        }

        void setY(int value) {
            y_ = value;
        }

        Point operator + (Point other) const {
            return Point{x_ + other.x_, y_ + other.y_};
        }

        Point operator - (Point other) const {
            return Point{x_ - other.x_, y_ - other.y_};
        }

        Point & operator += (Point other) {
            x_ += other.x_;
            y_ += other.y_;
            return *this;
        }

        Point & operator -= (Point other) {
            x_ -= other.x_;
            y_ -= other.y_;
            return *this;
        }

        bool operator >= (Point other) const {
            return x_ >= other.x_ && y_ >= other.y_;
        }

        bool operator < (Point other) const {
            return x_ < other.x_ && y_ < other.y_;
        }

        bool operator == (Point other) const {
            return (x_ == other.x_) && (y_ == other.y_);
        }

        bool operator != (Point other) const {
            return (x_ != other.x_) || (y_ != other.y_);
        }

        static Point MinCoordWise(Point const & a, Point const & b) {
            return Point{std::min(a.x(), b.x()), std::min(a.y(), b.y())};
        }

        static Point MaxCoordWise(Point const & a, Point const & b) {
            return Point{std::max(a.x(), b.x()), std::max(a.y(), b.y())};
        }

    private:

        int x_;
        int y_;

        friend std::ostream & operator << (std::ostream & s, Point const & p) {
            s << "[" << p.x_ << ", " << p.y_ << "]";
            return s;
        }

    }; // ui::Point



    class Rect {
    public:

	    Rect():
		    left_{0},
			top_{0},
			width_{0},
			height_{0} {
		}

        static Rect Empty() {
            return Rect{};
        }

		static Rect FromWH(int width, int height) {
			return Rect{0,0, width, height};
		}

	    static Rect FromTopLeftWH(Point const & topLeft, int width, int height) {
			return Rect{topLeft.x(), topLeft.y(), width, height};
		}

	    static Rect FromTopLeftWH(int left, int top, int width, int height) {
			return Rect{left, top, width, height};
		}

		static Rect FromCorners(int left, int top, int right, int bottom) {
			return Rect{left, top, right - left, bottom - top};
		}
		static Rect FromCorners(Point const & topLeft, Point const & bottomRight) {
			return Rect{topLeft.x(), topLeft.y(), bottomRight.x() - topLeft.x(), bottomRight.y() - topLeft.y()};
		}

		int left() const {
			return left_;
		}

		int top() const {
			return top_;
		}

		int right() const {
			return left_ + width_;
		}

		int bottom() const {
			return top_ + height_;
		}

		int width() const {
			return width_;
		}

		int height() const {
			return height_;
		}

        Point topLeft() const {
            return Point{left_, top_};
        }

        Point topRight() const {
            return Point{left_ + width_, top_};
        }

        Point bottomLeft() const {
            return Point{left_, top_ + height_};
        }

        Point bottomRight() const {
            return Point{left_ + width_, top_ + height_};
        }

        bool empty() const {
            return width_ == 0 || height_ == 0;
        }

        bool contains(Point p) const {
            return p >= topLeft() && p < bottomRight();
        }

		Rect operator + (Point const & by) const {
			return FromTopLeftWH(left_ + by.x(), top_ + by.y(), width_, height_);
		}

		/** Returns an intersection of two rectangles. 
		 */
		Rect operator & (Rect const & other) const {
			using namespace std;
			return Rect::FromCorners(
				max(left(), other.left()),
				max(top(), other.top()),
				min(right(), other.right()),
				min(bottom(), other.bottom())
			);
		}

		/** Returns the union of two rectangles. 
		 */
		Rect operator | (Rect const & other) const {
			using namespace std;
			return Rect::FromCorners(
				min(left(), other.left()),
				min(top(), other.top()),
				max(right(), other.right()),
				max(bottom(), other.bottom())
			);
		}

        bool operator == (Rect const & other) const {
            return (left_ == other.left_) && (top_ == other.top_) && (width_ == other.width_) && (height_ == other.height_);
        }

        bool operator != (Rect const & other) const {
            return (left_ != other.left_) || (top_ != other.top_) || (width_ != other.width_) || (height_ != other.height_);
        }

    private:
		Rect(int left, int top, int width, int height):
		    left_(left),
			top_(top),
			width_((width < 0) ? 0 : width),
			height_((height < 0) ? 0 : height) {
		}

        int left_;
        int top_;
        int width_;
        int height_;

    }; // ui::Rect

    class Color {
    public:
		uint8_t a;
		uint8_t b;
		uint8_t g;
		uint8_t r;
        
		/** Creates a color of given properties. 
		 */
		constexpr Color(unsigned char red = 0, unsigned char green = 0, unsigned char blue = 0, unsigned char alpha = 255) :
		    a(alpha),
			b(blue),
			g(green),
			r(red) {
		}

        uint32_t toRGB() const {
            return (r << 16) + (g << 8) + b;
        };

		uint32_t toRGBA() const {
			return * pointer_cast<uint32_t const*>(this);
		}

		Color withAlpha(unsigned char value) const {
			return Color{r, g, b, value};
		}

        float floatAlpha() {
            return static_cast<float>(a) / 255.0f;
        }

		/** Returns true if the color is opaque, i.e. its alpha channel is maximized. 
		 */
		bool opaque() const {
			return a == 255;
		}

		/** Bleds the current color oven existing one. 
		 */
		Color blendOver(Color other) const {
            // if the color to blend over is none, or our color is completely transparent preserve the other color
            if (other == Color::None || a == 0) {
                return other;
            } else if (a == 255) {
				return *this;
			} else { 
				unsigned char aa = a + 1;
				unsigned char aInv = static_cast<unsigned char>(256 - a);
				unsigned char rr = static_cast<unsigned char>((aa * r + aInv * other.r) / 256);
				unsigned char gg = static_cast<unsigned char>((aa * g + aInv * other.g) / 256);
				unsigned char bb = static_cast<unsigned char>((aa * b + aInv * other.b) / 256);
                // TODO this is probably only correct when the other color is opaque... 
				return Color{rr, gg, bb, other.a};
			}
		}

		bool operator == (Color const & other) const {
			return r == other.r && g == other.g && b == other.b && a == other.a;
		}

		bool operator != (Color const & other) const {
			return r != other.r || g != other.g || b != other.b || a != other.a;
		}

		/** \name Predefined static colors for convenience.
		 */
		//@{
		static Color const None;
		static Color const Black;
		static Color const White;
		static Color const Green;
		static Color const Blue;
		static Color const Red;
		static Color const Magenta;
		static Color const Cyan;
		static Color const Yellow;
		static Color const Gray;
		static Color const DarkGreen;
		static Color const DarkBlue;
		static Color const DarkRed;
		static Color const DarkMagenta;
		static Color const DarkCyan;
		static Color const DarkYellow;
		static Color const DarkGray;
		//@}

		/** Parses a color from its HTML definition.

		    The color string must be either RGB or RGBA format and should be preceded with `#` according to the specification. However the permissive parser does not require the hash prefix.   
		 */
		static Color FromHTML(std::string const & colorCode) {
			unsigned start = colorCode[0] == '#' ? 1 : 0;
			if (colorCode.size() - start < 6)
			    THROW(helpers::IOError()) << "Exepected at least RRGGBB color definition but " << colorCode << " found.";
			unsigned char r = static_cast<unsigned char>(helpers::ParseHexNumber(colorCode.c_str() + start, 2));
			unsigned char g = static_cast<unsigned char>(helpers::ParseHexNumber(colorCode.c_str() + start + 2, 2));
			unsigned char b = static_cast<unsigned char>(helpers::ParseHexNumber(colorCode.c_str() + start + 4, 2));
			unsigned char a = 0xff;
			if (colorCode.size() - start == 8)
				a = static_cast<unsigned char>(helpers::ParseHexNumber(colorCode.c_str() + start + 6, 2));
			else if (colorCode.size() - start != 6)
			    THROW(helpers::IOError()) << "Exepected at least RRGGBBAA color definition but " << colorCode << " found.";
			return Color(r, g, b ,a);
		}

    private:
        friend class Cell;

        // TODO maybe the order of RGB in the Color is not correct
        Color(uint32_t raw) {
            *pointer_cast<uint32_t *>(this) = raw;
        }
    }; // ui::Color

	inline std::ostream & operator << (std::ostream & s, Color const & c) {
		s << static_cast<unsigned>(c.r) << ";" << static_cast<unsigned>(c.g) << ";" << static_cast<unsigned>(c.b) << ";" << static_cast<unsigned>(c.a);
		return s;
	}

    /** Border properties. 
     
        Determines the border color and border types (none, thin or thick) for top, left, right and bottom borders. 
     */
    class Border {
    public:

        /** Border kind. 
         */
        enum class Kind {
            None,
            Thin,
            Thick
        }; // ui::Border::Kind

        Border(Color color = Color::None):
            color_{color},
            border_{0} {
        }

        Color color() const {
            return color_;
        }

        Border & setColor(Color color) {
            color_ = color;
            return *this;
        }

        Kind left() const {
            return static_cast<Kind>((border_ >> LEFT) & MASK);
        }

        Kind right() const {
            return static_cast<Kind>((border_ >> RIGHT) & MASK);
        }

        Kind top() const {
            return static_cast<Kind>((border_ >> TOP) & MASK);
        }

        Kind bottom() const {
            return static_cast<Kind>((border_ >> BOTTOM) & MASK);
        }

        Border & setLeft(Kind kind) {
            border_ = (border_ & (~ (MASK << LEFT))) | (static_cast<uint16_t>(kind) << LEFT); 
            return *this;
        }

        Border & setRight(Kind kind) {
            border_ = (border_ & (~ (MASK << RIGHT))) | (static_cast<uint16_t>(kind) << RIGHT); 
            return *this;
        }

        Border & setTop(Kind kind) {
            border_ = (border_ & (~ (MASK << TOP))) | (static_cast<uint16_t>(kind) << TOP); 
            return *this;
        }

        Border & setBottom(Kind kind) {
            border_ = (border_ & (~ (MASK << BOTTOM))) | (static_cast<uint16_t>(kind) << BOTTOM); 
            return *this;
        }

        Border & setAll(Kind kind) {
            setLeft(kind);
            setRight(kind);
            setTop(kind);
            setBottom(kind);
            return *this;
        }

        Border & clear() {
            border_ = 0;
            return *this;
        }

        bool empty() const {
            return border_ == 0 || color_ == Color::None;
        }

        Border & updateWith(Border const & other) {
            color_ = other.color_;
            if (other.top() != Kind::None)
                setTop(other.top());
            if (other.left() != Kind::None)
                setLeft(other.left());
            if (other.bottom() != Kind::None)
                setBottom(other.bottom());
            if (other.right() != Kind::None)
                setRight(other.right());
            return *this;
        }

        bool operator == (Border const & other) const {
            return color_ == other.color_ && border_ == other.border_;
        }

        bool operator != (Border const & other) const {
            return color_ != other.color_ || border_ != other.border_;
        }

    private:
        static constexpr uint16_t MASK = 0x03;
        static constexpr uint16_t LEFT = 0;
        static constexpr uint16_t RIGHT = 2;
        static constexpr uint16_t TOP = 4;
        static constexpr uint16_t BOTTOM = 6;

        Color color_;
        uint16_t border_;
    }; // ui::Border

    /** Font properties. 
     
        Contains the information about the font size, type and decorations. 
     */
    class Font {
    public:

        Font():
            font_{0} {
        }

        bool bold() const {
            return font_ & BOLD;
        }

        Font & setBold(bool value = true) {
            font_ = helpers::SetBit(font_, BOLD, value);
            return *this;
        }

        bool italic() const {
            return font_ & ITALIC;
        }

        Font & setItalic(bool value = true) {
            font_ = helpers::SetBit(font_, ITALIC, value);
            return *this;
        }

        bool underline() const {
            return font_ & UNDERLINE;
        }

        Font & setUnderline(bool value = true) {
            font_ = helpers::SetBit(font_, UNDERLINE, value);
            return *this;
        }

        bool strikethrough() const {
            return font_ & STRIKETHROUGH;
        }

        Font & setStrikethrough(bool value = true) {
            font_ = helpers::SetBit(font_, STRIKETHROUGH, value);
            return *this;
        }

        bool blink() const {
            return font_ & BLINK;
        }

        Font & setBlink(bool value = true) {
            font_ = helpers::SetBit(font_, BLINK, value);
            return *this;
        }
        bool doubleWidth() const {
            return font_ & DOUBLE_WIDTH;
        }

        Font & setDoubleWidth(bool value = true) {
            font_ = helpers::SetBit(font_, DOUBLE_WIDTH, value);
            return *this;
        }

        int size() const {
            return (font_ & SIZE_MASK) + 1;
        }

        Font & setSize(int size) {
            size -= 1;
            ASSERT(size >= 0 && size < 8);
            font_ = helpers::SetBits(font_, SIZE_MASK, static_cast<uint16_t>(size));
            return *this;
        }

        int width() const {
            return doubleWidth() ? size() * 2 : size();
        }

        int height() const {
            return size();
        }

        bool operator == (Font const & other) const {
            return font_ == other.font_;
        } 

        bool operator != (Font const & other) const {
            return font_ != other.font_;
        }

    private:
        static constexpr uint16_t BOLD = 1 << 15;
        static constexpr uint16_t ITALIC = 1 << 14;
        static constexpr uint16_t UNDERLINE = 1 << 13;
        static constexpr uint16_t STRIKETHROUGH = 1 << 12;
        static constexpr uint16_t BLINK = 1 << 11;
        static constexpr uint16_t DOUBLE_WIDTH = 1 << 10;


        static constexpr uint16_t SIZE_MASK = 7;

        uint16_t font_;

    }; // ui::Font



    /** Fill brush. 
        
     */
    class Brush {
    public:

        Brush():
            fillChar_{0x20},
            fillColor_{Color::None} {
        }

        Brush(Color color, char32_t fillChar = 0x20):
            color_{color},
            fillChar_{fillChar},
            fillColor_{Color::None} {
        }

        /** Returns the background color of the brush. 
         */
        Color color() const {
            return color_;
        }

        Brush & setColor(Color value) {
            color_ = value; 
            return *this;
        }

        Font fillFont() const {
            return fillFont_;
        }

        Brush & setFillFont(Font value) {
            fillFont_ = value;
            return *this;
        }

        char32_t fillChar() const {
            return fillChar_;
        }

        Brush & setFillChar(char32_t value) {
            fillChar_ = value;
            return *this;
        }

        Color fillColor() const {
            return fillColor_;
        }

        Brush & setFillColor(Color value) {
            fillColor_ = value;
            return *this;
        }

        bool operator == (Brush const & other) const {
            return color_ == other.color_ && fillFont_ == other.fillFont_ && fillChar_ == other.fillChar_ && fillColor_ == other.fillColor_;
        }

        bool operator != (Brush const & other) const {
            return color_ != other.color_ || fillFont_ != other.fillFont_ || fillChar_ != other.fillChar_ || fillColor_ != other.fillColor_;
        }

    private:

        Color color_;
        Font fillFont_;
        char32_t fillChar_;
        Color fillColor_;

    }; // ui::Brush


} // namespace ui
#pragma once

#include <cstdint>
#include <algorithm>

#include "helpers/helpers.h"
#include "helpers/string.h"

namespace ui2 {

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

        Point operator + (Point other) const {
            return Point{x_ + other.x_, y_ + other.y_};
        }

        Point operator - (Point other) const {
            return Point{x_ - other.x_, y_ - other.y_};
        }

        bool operator >= (Point other) const {
            return x_ >= other.x_ && y_ >= other.y_;
        }

        bool operator < (Point other) const {
            return x_ < other.x_ && y_ < other.y_;
        }

    private:

        int x_;
        int y_;

    }; // ui::Point



    class Rect {
    public:

	    Rect():
		    left_{0},
			top_{0},
			width_{0},
			height_{0} {
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
		Color blendOver(Color const& other) const {
			if (a == 255) {
				return *this;
			} else if (a == 0) {
				return other;
			} else if (other.a == 255) {
				unsigned char aa = a + 1;
				unsigned char aInv = static_cast<unsigned char>(256 - a);
				unsigned char rr = static_cast<unsigned char>((aa * r + aInv * other.r) / 256);
				unsigned char gg = static_cast<unsigned char>((aa * g + aInv * other.g) / 256);
				unsigned char bb = static_cast<unsigned char>((aa * b + aInv * other.b) / 256);
				return Color(rr, gg, bb, 255);
			} else {
				// TODO we can do this because the color always blends over an existing fully opaque color of the background.If this were not the case, the assert fails and we have to change the algorithm.
				UNREACHABLE;
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

} // namespace ui
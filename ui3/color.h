#pragma once

#include "helpers/helpers.h"
#include "helpers/string.h"

namespace ui3 {

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
			    THROW(IOError()) << "Exepected at least RRGGBB color definition but " << colorCode << " found.";
			unsigned char r = static_cast<unsigned char>(ParseHexNumber(colorCode.c_str() + start, 2));
			unsigned char g = static_cast<unsigned char>(ParseHexNumber(colorCode.c_str() + start + 2, 2));
			unsigned char b = static_cast<unsigned char>(ParseHexNumber(colorCode.c_str() + start + 4, 2));
			unsigned char a = 0xff;
			if (colorCode.size() - start == 8)
				a = static_cast<unsigned char>(ParseHexNumber(colorCode.c_str() + start + 6, 2));
			else if (colorCode.size() - start != 6)
			    THROW(IOError()) << "Exepected at least RRGGBBAA color definition but " << colorCode << " found.";
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
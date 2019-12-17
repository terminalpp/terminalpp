#pragma once

#include <cstdint>

#include "helpers/helpers.h"
#include "helpers/string.h"

namespace ui {

    class Color {
    public:
		uint8_t alpha;
		uint8_t blue;
		uint8_t green;
		uint8_t red;
        
		/** Creates a color of given properties. 
		 */
		constexpr Color(unsigned char red = 0, unsigned char green = 0, unsigned char blue = 0, unsigned char alpha = 255) :
		    alpha(alpha),
			blue(blue),
			green(green),
			red(red) {
		}

        uint32_t toRGB() const {
            return (red << 16) + (green << 8) + blue;
        };

		uint32_t toRGBA() const {
			return * helpers::pointer_cast<uint32_t const*>(this);
		}

		Color withAlpha(unsigned char value) const {
			return Color{red, green, blue, value};
		}

        float floatAlpha() {
            return static_cast<float>(alpha) / 255.0f;
        }

		/** Returns true if the color is opaque, i.e. its alpha channel is maximized. 
		 */
		bool opaque() const {
			return alpha == 255;
		}

		/** Bleds the current color oven existing one. 
		 */
		Color blendOver(Color const& other) const {
			if (alpha == 255) {
				return *this;
			} else if (alpha == 0) {
				return other;
			} else if (other.alpha == 255) {
				unsigned char a = alpha + 1;
				unsigned char aInv = static_cast<unsigned char>(256 - alpha);
				unsigned char r = static_cast<unsigned char>((a * red + aInv * other.red) / 256);
				unsigned char g = static_cast<unsigned char>((a * green + aInv * other.green) / 256);
				unsigned char b = static_cast<unsigned char>((a * blue + aInv * other.blue) / 256);
				return Color(r, g, b, 255);
			} else {
				// TODO we can do this because the color always blends over an existing fully opaque color of the background.If this were not the case, the assert failsand we have to change the algorithm.
				NOT_IMPLEMENTED;
			}
		}

		bool operator == (Color const & other) const {
			return red == other.red && green == other.green && blue == other.blue && alpha == other.alpha;
		}

		bool operator != (Color const & other) const {
			return red != other.red || green != other.green || blue != other.blue || alpha != other.alpha;
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
            *helpers::pointer_cast<uint32_t *>(this) = raw;
        }

    }; 

	inline std::ostream & operator << (std::ostream & s, Color const & c) {
		s << static_cast<unsigned>(c.red) << ";" << static_cast<unsigned>(c.green) << ";" << static_cast<unsigned>(c.blue);
		return s;
	}

	struct ForegroundColorHolder {
	    Color value;
	};

	struct BackgroundColorHolder {
	    Color value;
	};

	struct DecorationColorHolder {
	    Color value;
	};

	struct BorderColorHolder {
	    Color value;
	};

	inline ForegroundColorHolder Foreground(Color color) {
		return ForegroundColorHolder{color};
	}

	inline BackgroundColorHolder Background(Color color) {
		return BackgroundColorHolder{color};
	}

	inline DecorationColorHolder DecorationColor(Color color) {
		return DecorationColorHolder{color};
	}

	inline BorderColorHolder BorderColor(Color color) {
		return BorderColorHolder{color};
	}

} // namespace ui
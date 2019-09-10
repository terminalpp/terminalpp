#pragma once

#include <cstdint>

#include "helpers/helpers.h"

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

		Color & setAlpha(unsigned char value) {
			alpha = value;
			return *this;
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

		/** Predefined static colors for convenience.

		    They have to me static methods because of the static initialization order issue.
		 */
		static Color None() { return Color(0, 0, 0, 0); }
		static Color Black() { return Color(0, 0, 0); }
		static Color White() { return Color(255, 255, 255);	}
		static Color Green() { return Color(0, 255, 0);	}
		static Color Blue() { return Color(0, 0, 255);	}
		static Color Red() { return Color(255, 0, 0); }
		static Color Magenta() { return Color(255, 0, 255); }
		static Color Cyan() { return Color(0, 255, 255); }
		static Color Yellow() { return Color(255, 255, 0); }
		static Color Gray() { return Color(196, 196, 196); }
		static Color DarkGreen() { return Color(0, 128, 0); }
		static Color DarkBlue() { return Color(0, 0, 128); }
		static Color DarkRed() { return Color(128, 0, 0); }
		static Color DarkMagenta() { return Color(128, 0, 128); }
		static Color DarkCyan() { return Color(0, 128, 128); }
		static Color DarkYellow() { return Color(128, 128, 0); }
		static Color DarkGray() { return Color(128, 128, 128); }

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

	inline ForegroundColorHolder Foreground(Color color) {
		return ForegroundColorHolder{color};
	}

	inline BackgroundColorHolder Background(Color color) {
		return BackgroundColorHolder{color};
	}

	inline DecorationColorHolder DecorationColor(Color color) {
		return DecorationColorHolder{color};
	}

} // namespace ui
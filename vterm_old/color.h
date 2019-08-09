#pragma once

#include <ostream>

#include "helpers/helpers.h"

namespace vterm {

	/** Color specification (8bit true color with alpha channel)
	 */
	class Color {
	public:
		/** Red channel. 
		 */
		unsigned char red;
		
		/** Green channel.
		 */
		unsigned char green;
		
		/** Blue channel. 
		 */
		unsigned char blue;

		/** Alpha channel - 0 is no transparency, 255 is full transparency. 
		 */
		unsigned char alpha;

		/** Creates a color of given properties. 
		 */
		constexpr Color(unsigned char red, unsigned char green, unsigned char blue, unsigned char alpha = 255) :
			red(red),
			green(green),
			blue(blue),
			alpha(alpha) {
		}

		/** Default constructor creates black color.
		 */
		constexpr Color() :
			red(0),
			green(0),
			blue(0),
			alpha(255) {
		}

		unsigned toNumber() const {
			return (red << 16) + (green << 8) + blue;
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
	};

	inline std::ostream & operator << (std::ostream & s, Color const & c) {
		s << static_cast<unsigned>(c.red) << ";" << static_cast<unsigned>(c.green) << ";" << static_cast<unsigned>(c.blue);
		return s;
	}

	/** Palette of colors.

		Although vterm fully supports the true color rendering, for compatibility and shorter escape codes, the 256 color palette as defined for xterm is supported via this class.

		The separation of the palette and the terminal allows very simple theming in the future, should this feature be implemented.
	 */
	class Palette {
	public:
		Palette(size_t size) :
			size_(size),
			colors_(new Color[size]) {
		}

		Palette(std::initializer_list<Color> colors);

		Palette(Palette const& from);

		Palette(Palette&& from);

		~Palette() {
			delete [] colors_;
		}

		void fillFrom(Palette const& from);

		size_t size() const {
			return size_;
		}

		Color const& operator [] (size_t index) const {
			return color(index);
		}

		Color& operator [] (size_t index) {
			return color(index);
		}

		Color const& color(size_t index) const {
			ASSERT(index < size_);
			return colors_[index];
		}

		Color& color(size_t index) {
			ASSERT(index < size_) << index << " -- " << size_;
			return colors_[index];
		}

		static Palette Colors16();

	private:
		size_t size_;
		Color* colors_;
	};
} // namespace vterm
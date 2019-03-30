#pragma once

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
		constexpr Color(unsigned char red, unsigned char green, unsigned char blue, unsigned char alpha = 0) :
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
			alpha(0) {
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

} // namespace vterm
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
		Color(unsigned char red, unsigned char green, unsigned char blue, unsigned char alpha = 0) :
			red(red),
			green(green),
			blue(blue),
			alpha(alpha) {
		}

		/** Default constructor creates black color.
		 */
		Color() :
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
		 */
		static Color const Black;
		static Color const White;
		static Color const Green;
		static Color const Blue;
		static Color const Red;
		static Color const Magenta;
		static Color const Cyan;
		static Color const Yellow;
		static Color const Gray;
	};

} // namespace vterm
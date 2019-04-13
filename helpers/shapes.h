#pragma once

#include <algorithm>

#ifdef WIN32
#undef min
#undef max
#endif

namespace helpers {
	

	// TODO this should move to vterm because the shapes are very much terminal oriented only

	/** Point definition. 
	 */
	class Point {
	public:
		unsigned col;
		unsigned row;

		Point(unsigned col, unsigned row) :
			col(col),
			row(row) {
		}
	};

	/** Rectangle definition.

	    The rectange is assumed to be inclusive of its left and top coordinates and exclusive for its bottom and right coordinates. 
	 */
	class Rect {
	public:
		unsigned left;
		unsigned top;
		unsigned right;
		unsigned bottom;

		unsigned cols() const {
			return right - left;
		}

		unsigned rows() const {
			return bottom - top;
		}

		bool empty() const {
			return cols() == 0 || rows() == 0;
		}

		bool operator == (Rect const & other) const {
			return left == other.left && top == other.top && right == other.right && bottom == other.bottom;
		}

		bool operator != (Rect const & other) const {
			return left != other.left || top != other.top || right != other.right || bottom != other.bottom;
		}

		Rect(unsigned width, unsigned height) :
			left(0),
			top(0),
			right(width),
			bottom(height) {
		}

		Rect(unsigned left, unsigned top, unsigned right, unsigned bottom) :
			left(left),
			top(top),
			right(right),
			bottom(bottom) {
			if (right < left)
				right = left;
			if (bottom < top)
				bottom = top;
		}

		/** Returns a rectangle formed by union of two existing rectangles, i.e. a rectangle large enough to encompass both.
		 */
		static Rect Union(Rect const & first, Rect const & second) {
			if (first.empty())
				return second;
			else if (second.empty())
				return first;
			else
				return Rect{
					std::min(first.left, second.left),
					std::min(first.top, second.top),
					std::max(first.right, second.right),
					std::max(first.bottom, second.bottom)
			    };
		}

		/** Returns the intersection of the two rectangles.
		 */
		static Rect Intersection(Rect const & first, Rect const & second) {
			if (first.empty())
				return first;
			else if (second.empty())
				return second;
			else
				return Rect{
					std::max(first.left, second.left),
					std::max(first.top, second.top),
					std::min(first.right, second.right),
					std::min(first.bottom, second.bottom)
			    };
		}

	}; // helpers::Rect

} // namespace helpers
#pragma once

#include <algorithm>
#include <ostream>

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

		Point() :
			col(0),
			row(0) {
		}

		Point(unsigned col, unsigned row) :
			col(col),
			row(row) {
		}

		bool operator == (Point const& other) const {
			return col == other.col && row == other.row;
		}

		bool operator != (Point const& other) const {
			return col != other.col || row != other.row;
		}

		friend std::ostream& operator << (std::ostream& s, Point const& p) {
			s << "[" << p.col << "," << p.row << "]";
			return s;
		}
	};

	static_assert(sizeof(Point) == sizeof(unsigned) * 2, "Point is supposed to be two unsigned numbers only");


	/** Rectangle definition.

	    The rectange is assumed to be inclusive of its left and top coordinates and exclusive for its bottom and right coordinates. 
	 */
	class Rect {
	public:
		unsigned left;
		unsigned top;
		unsigned right;
		unsigned bottom;

		Point const& topLeft() const {
			return reinterpret_cast<Point const*>(this)[0];
		}

		Point const& bottomRight() const {
			return reinterpret_cast<Point const*>(this)[1];
		}

		Point & topLeft() {
			return reinterpret_cast<Point*>(this)[0];
		}

		Point & bottomRight() {
			return reinterpret_cast<Point*>(this)[1];
		}

		unsigned cols() const {
			return right - left;
		}

		unsigned rows() const {
			return bottom - top;
		}

		bool empty() const {
			return cols() == 0 && rows() == 0;
		}

		bool operator == (Rect const & other) const {
			return left == other.left && top == other.top && right == other.right && bottom == other.bottom;
		}

		bool operator != (Rect const & other) const {
			return left != other.left || top != other.top || right != other.right || bottom != other.bottom;
		}

		Rect() :
			left(0),
			top(0),
			right(0),
			bottom(0) {
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
				std::swap(right,left);
			if (bottom < top)
				std::swap(bottom, top);
		}

		Rect(Point const& topLeft, Point const& bottomRight) :
			left(topLeft.col),
			top(topLeft.row),
			right(bottomRight.col),
			bottom(bottomRight.row) {
			if (right < left)
				std::swap(right, left);
			if (bottom < top)
				std::swap(bottom, top);
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

	static_assert(sizeof(Rect) == sizeof(Point) * 2, "Point & rect size mismatch, topLeft and bottomRight won't work");

} // namespace helpers
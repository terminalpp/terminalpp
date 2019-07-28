#pragma once

#include <algorithm>
#include <ostream>

namespace helpers {
	

	/** Point definition. 
	 */
	template<typename COORD>
	class Point {
	public:
		COORD col;
		COORD row;

		Point() :
			col(0),
			row(0) {
		}

		Point(COORD col, COORD row) :
			col(col),
			row(row) {
			static_assert(sizeof(Point) == sizeof(COORD) * 2, "Point is supposed to be two numbers only");
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

	/** Rectangle definition.

	    The rectange is assumed to be inclusive of its left and top coordinates and exclusive for its bottom and right coordinates. 
	 */
	template<typename COORD>
	class Rect {
	public:
		COORD left;
		COORD top;
		COORD right;
		COORD bottom;

		Point<COORD> const& topLeft() const {
			return reinterpret_cast<Point<COORD> const*>(this)[0];
		}

		Point<COORD> const& bottomRight() const {
			return reinterpret_cast<Point<COORD> const*>(this)[1];
		}

		Point<COORD> & topLeft() {
			return reinterpret_cast<Point<COORD>*>(this)[0];
		}

		Point<COORD> & bottomRight() {
			return reinterpret_cast<Point<COORD>*>(this)[1];
		}

		COORD width() const {
			return right - left;
		}

		COORD height() const {
			return bottom - top;
		}

		/** Returns true if the rectangle contains given point. 
		 */
		bool contains(Point<COORD> point) const {
			return (point.col >= left) && (point.row >= top) && (point.col < right) && (point.row < bottom);
		}

		bool empty() const {
			return width() == 0 && height() == 0;
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
			static_assert(sizeof(Rect<COORD>) == sizeof(Point<COORD>) * 2, "Point & rect size mismatch, topLeft and bottomRight won't work");
		}

		Rect(COORD width, COORD height) :
			left(0),
			top(0),
			right(width),
			bottom(height) {
		}

		Rect(COORD left, COORD top, COORD right, COORD bottom) :
			left(left),
			top(top),
			right(right),
			bottom(bottom) {
			if (right < left)
				std::swap(right,left);
			if (bottom < top)
				std::swap(bottom, top);
		}

		Rect(Point<COORD> const& topLeft, Point<COORD> const& bottomRight) :
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
			if (first.empty() || second.empty()) {
				return Rect(0, 0);
			} else {
				COORD left = std::max(first.left, second.left);
				COORD top = std::max(first.top, second.top);
				COORD right = std::min(first.right, second.right);
				COORD bottom = std::min(first.bottom, second.bottom);
				if (left < right && top < bottom)
					return Rect(left, top, right, bottom);
				else
					return Rect(0, 0);
			}
		}

		Rect& operator += (Point<COORD> const & point) {
			left += point.col;
			top += point.row;
			right += point.col;
			bottom += point.row;
			return *this;
		}

		friend Rect operator + (Rect const& rect, Point<COORD> const& p) {
			return Rect(rect.left + p.col, rect.top + p.row, rect.right + p.col, rect.bottom + p.row);
		}

		/** Moves the rectangle by coordinates given by the point. 
		 */
		friend Rect operator - (Rect const& rect, Point<COORD> const& p) {
			return Rect(rect.left - p.col, rect.top - p.row, rect.right - p.col, rect.bottom - p.row);
		}


	}; // helpers::Rect

} // namespace helpers
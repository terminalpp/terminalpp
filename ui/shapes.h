#pragma once

#include <algorithm>

#include "helpers/char.h"

#include "color.h"
#include "font.h"

namespace ui {

    class Point {
    public:
        int x;
        int y;

        Point(int x = 0, int y = 0):
            x(x), 
            y(y) {
        }

		bool operator == (Point const& other) const {
			return x == other.x && y == other.y;
		}

		bool operator != (Point const& other) const {
			return x != other.x || y != other.y;
		}

		Point& operator += (Point const& other) {
			x += other.x;
			y += other.y;
			return *this;
		}

		friend std::ostream& operator << (std::ostream& s, Point const& p) {
			s << "[" << p.x << "," << p.y << "]";
			return s;
		}

    }; 

    class Rect {
    public:
        
        Point topLeft;
        Point bottomRight;

        Rect():
            topLeft(0,0),
            bottomRight(0,0) {
        }

        Rect(Point topLeft, Point bottomRight):
            topLeft(topLeft),
            bottomRight(bottomRight) {
        }

		// TODO check the usage of this, perhaps use static 
        Rect(int left, int top, int right, int bottom):
            topLeft(left, top),
            bottomRight(right, bottom) {
        }

        Rect(int width, int height):
            topLeft(0, 0),
            bottomRight(width, height) {
        }

        int left() const {
            return topLeft.x;
        }

        int top() const {
            return topLeft.y;
        }

        int right() const {
            return bottomRight.x;
        }

        int bottom() const {
            return bottomRight.y;
        }

        int width() const {
            return bottomRight.x - topLeft.x;
        }

        int height() const {
            return bottomRight.y - topLeft.y;
        }

		Point bottomLeft() const {
			return Point(topLeft.x, bottomRight.y);
		}

		Point topRight() const {
			return Point(bottomRight.x, topLeft.y);
		}

		/** Returns true if the rectangle contains given point. 
		 */
		bool contains(Point point) const {
			return (point.x >= left()) && (point.y >= top()) && (point.x < right()) && (point.y < bottom());
		}

		bool empty() const {
			return width() == 0 && height() == 0;
		}

		bool operator == (Rect const & other) const {
			return topLeft == other.topLeft && bottomRight == other.bottomRight;
		}

		bool operator != (Rect const & other) const {
			return topLeft != other.topLeft || bottomRight != other.bottomRight;
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
					std::min(first.left(), second.left()),
					std::min(first.top(), second.top()),
					std::max(first.right(), second.right()),
					std::max(first.bottom(), second.bottom())
			    };
		}

		/** Returns the intersection of the two rectangles.
		 */
		static Rect Intersection(Rect const & first, Rect const & second) {
			if (first.empty() || second.empty()) {
				return Rect(0, 0);
			} else {
				int left = std::max(first.left(), second.left());
				int top = std::max(first.top(), second.top());
				int right = std::min(first.right(), second.right());
				int bottom = std::min(first.bottom(), second.bottom());
				if (left < right && top < bottom)
					return Rect(left, top, right, bottom);
				else
					return Rect(0, 0);
			}
		}

		Rect& operator += (Point const & point) {
            topLeft += point;
            bottomRight += point;
			return *this;
		}

		friend Rect operator + (Rect const& rect, Point const& p) {
			return Rect(rect.left() + p.x, rect.top() + p.y, rect.right() + p.x, rect.bottom() + p.y);
		}

		/** Moves the rectangle by coordinates given by the point. 
		 */
		friend Rect operator - (Rect const& rect, Point const& p) {
			return Rect(rect.left() - p.x, rect.top() - p.y, rect.right() - p.x, rect.bottom() - p.y);
		}

		friend std::ostream & operator << (std::ostream & s, Rect const & rect) {
			s << "[" << rect.left() << "," << rect.top() << "; " << rect.right() << "," << rect.bottom() << "]";
			return s;
		}


    };

	class Border {
	public:
		int left;
		int top;
		int right;
		int bottom;
		
		Border(int left = 0, int top = 0, int right = 0, int bottom = 0) :
			left(left),
			top(top),
			right(right),
			bottom(bottom) {
		}

		bool operator == (Border const& other) const {
			return left == other.left && top == other.top && right == other.right && bottom == other.bottom;
		}

		bool operator != (Border const& other) const {
			return left != other.left || top != other.top || right != other.right || bottom != other.bottom;
		}

	};


	/** A brush used to fill areas on the canvas. 

	    A brush consists of background color which is applied as backrgound colors to the cells, and the fill character and its color which can be written in the respective cells. 
	 */
	class Brush {
	public:

		/** The background color of the brush. 

		    All cells background color will be set to this color. This color can be transparent. 
		 */
		Color color;

		/** Fill character. 
		 */
		char32_t fill;

		/** Color of the fill character. 
		 */
		Color fillColor;

		/** Font of the fill character. 

		    Note that only fonts of size 1 are supported. 
			
			TODO how does this change with double width characters, would solution to this make it work with larger fonts as well? 
		 */
		Font fillFont;

		/** Creates a simple brush with only a background color. 

		    If the background color is not opaque, fill character is set to space, otherwise the fill character is set to NUL and its color to None. This means that if the background color is transparent, the contents of the cell will be kept as is, otherwise the cell will be erased.
		 */
		Brush(Color color) :
			color(color),
			fill(color.alpha == 255 ? ' ' : helpers::Char::NUL),
			fillColor(Color::None()),
		    fillFont(Font()) {
		}

		/** Creates a brush with specified fill character and its color. 
		
		    Such a brush will first change the background color, but then also overwrite the contents of the cell.	    
		 */
		Brush(Color color, char32_t fill, Color fillColor, Font fillFont = Font()) :
			color(color),
			fill(fill),
			fillColor(fillColor),
		    fillFont(fillFont) {
		}

		/** Returns an empty brush, which when used leaves all properties of the cell intact. 
		 */
		static Brush None() {
			return Brush(Color::None(), 0, Color::None());
		}

		/** Brushes are equal if their contents are equal. 
		 */
		bool operator == (Brush const& other) const {
			return color == other.color && fill == other.fill && fillColor == other.fillColor && fillFont == other.fillFont;
		}

		bool operator != (Brush const& other) const {
			return color != other.color || fill != other.fill || fillColor != other.fillColor || fillFont != other.fillFont;
		}
	}; // ui::Brush



} // namespace ui
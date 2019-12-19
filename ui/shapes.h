#pragma once

#include <algorithm>

#include "helpers/char.h"

#include "color.h"
#include "font.h"

namespace ui {

    /** 2D point with integer coordinates. 
     
        Supports basic point arithmetics - i.e. adding and subtracting points and numbers. 
     */
	class Point {
	public:

        int x;
        int y;

	    Point(int x = 0, int y = 0):
            x(x),
            y(y) {
        }

        bool isOrigin() const {
            return x == 0 && y == 0;
        }

        void set(int xx, int yy) {
            x = xx;
            y = yy;
        }

        bool operator == (Point const & other) const {
            return x == other.x && y == other.y;
        }

        bool operator != (Point const & other) const {
            return x != other.x || y != other.y;
        }

        Point operator + (Point const & other) const {
            return Point(x + other.x, y + other.y);
        }

        Point operator + (int scalar) const {
            return Point(x + scalar, y + scalar);
        }

        Point & operator += (Point const & other) {
            x += other.x;
            y += other.y;
            return *this;
        }

        Point & operator += (int scalar) {
            x += scalar;
            y += scalar;
            return *this;
        }

        Point operator - (Point const & other) const {
            return Point(x - other.x, y - other.y);
        }

        Point operator - (int scalar) const {
            return Point(x - scalar, y - scalar);
        }

        Point & operator -= (Point const & other) {
            x -= other.x;
            y -= other.y;
            return *this;
        }

        Point & operator -= (int scalar) {
            x -= scalar;
            y -= scalar;
            return *this;
        }

        friend std::ostream & operator << (std::ostream & s, Point const & p) {
            s << "[" << p.x << "," << p.y << "]";
            return s;
        }
	};
	
/** Rectangle. 
	 */	
	class Rect {
	public:

        /** \name Rectangle Constructors
         
            Because rectangles can be specified using different inputs, instead of constructors, static methods with disambiguating names are used instead. 
         */
        //@{

        /** The default constructor creates an empty rectangle. 
         */
        Rect():
            left_{0},
            top_{0},
            width_{0},
            height_{0} {
        }

	    static Rect Empty() {
            return Rect{};
        }

		static Rect FromCorners(Point topLeft, Point bottomRight) {
            int width = bottomRight.x - topLeft.x;
            int height = bottomRight.y - topLeft.y;
            return Rect{topLeft.x, topLeft.y, width, height};
        }

        static Rect FromCorners(int x1, int y1, int x2, int y2) {
            return Rect{x1, y1, x2 - x1, y2 - y1};
        }

		static Rect FromTopLeftWH(Point topLeft, int width, int height) {
            return Rect{topLeft.x, topLeft.y, width, height};
        }

		static Rect FromTopLeftWH(int x, int y, int width, int height) {
            return Rect{x, y, width, height};
        }

        static Rect FromWH(int width, int height) {
            return Rect{0, 0, width, height};
        }

        static Rect FromWH(Point dim) {
            return Rect{0, 0, dim.x, dim.y};
        }

        //@}

        bool empty() const {
            return width_ == 0 || height_ == 0;
        }

        /** \name Accessors 
         */
        //@{

	    int left() const { 
            return left_; 
        }
		
        int top() const { 
            return top_;
        }
	    int width() const { 
            return width_; 
        }
		int height() const { 
            return height_; 
        }
		int right() const { 
            return left_ + width_;
        }
		int bottom() const { 
            return top_ + height_;
        }

		Point topLeft() const { 
            return Point{left_, top_}; 
        }

		Point bottomRight() const {
            return Point{left_ + width_, top_ + height_};
        }
        //@}

        /** \name Setters
         */
        //@{
        /** Sets the left coordinate keeping the width intact. 
         */
        void setLeft(int value) {
            left_ = value;
        }

        void setTop(int value) {
            top_ = value;
        }

        void setWidth(int value) {
            width_ = value;
        }

        void setHeight(int value) {
            height_ = value;
        }
        
        //@}


		bool operator == (Rect const & other) const {
            return left_ == other.left_ && top_ == other.top_ && width_ == other.width_ && height_ == other.height_;
        }

		bool operator != (Rect const & other) const {
            return left_ != other.left_ || top_ != other.top_ || width_ != other.width_ || height_ != other.height_;
        }

        Rect & operator += (Point const & other) {
            left_ += other.x;
            top_ += other.y;
            return *this;
        }

        Rect operator + (Point const & other) const {
            return Rect{
                left_ + other.x,
                top_ + other.y,
                width_,
                height_
            };
        }

        Rect operator - (Point const & other) {
            return Rect{
                left_ - other.x,
                top_ - other.y,
                width_,
                height_
            };
        }

        /** Determines whether the rectangle contains the given point. 
         
            
         */
        bool contains(Point const & p) const {
            return left() <= p.x && right() > p.x && top() <= p.y && bottom() > p.y;
        }

        static Rect Intersection(Rect const & a, Rect const & b) {
            return FromCorners(
                Point{std::max(a.left_, b.left_),std::max(a.top_, b.top_)},
                Point{std::min(a.left_ + a.width_, b.left_ + b.width_), std::min(a.top_ + a.height_, b.top_ + b.height_)}
            );
        }

        static Rect Union(Rect const & a, Rect const & b) {
            if (a.empty())
                return b;
            else if (b.empty())
                return a;
            else 
                return FromCorners(
                    Point{std::min(a.left_, b.left_),std::min(a.top_, b.top_)},
                    Point{std::max(a.left_ + a.width_, b.left_ + b.width_), std::max(a.top_ + a.height_, b.top_ + b.height_)}
                );
        }

        friend std::ostream & operator << (std::ostream & s, Rect const & rect) {
            s << "[" << rect.topLeft() << "," << rect.bottomRight() << "]";
            return s;
        }
    
    private:

        Rect(int left, int top, int width, int height):
            left_(left),
            top_(top),
            width_(width),
            height_(height) {
            if (width < 0 || height < 0) {
                width_ = 0;
                height_ = 0;
            }
        } 

        int left_;
        int top_;
        int width_;
        int height_;

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
			fill(color.a == 255 ? ' ' : helpers::Char::NUL),
			fillColor(Color::None),
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
			return Brush(Color::None, 0, Color::None);
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
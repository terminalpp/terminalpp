#pragma once

#include "helpers/shapes.h"

#include "vterm/terminal.h"

namespace ui {

	/* Forward declarations */
	class RootWindow;

	/* For simplicity, bring existing types to the ui namespace where appropriate
	 */

	typedef helpers::Point<int> Point;
	typedef helpers::Rect<int> Rect;
	typedef vterm::Color Color;
	typedef vterm::Font Font;
	typedef helpers::Char Char;

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
	
	/** Horizontal align. 
	 */
	enum class HAlign {
		Left,
		Center,
		Right
	};

	/** Vertical Align 
	 */
	enum class VAlign {
		Top,
		Middle,
		Bottom
	};



	/** Describes the canvas of the UI controls.

		The canvas acts as an interface to the underlying root window. 
	 */
	class Canvas {
	public:

		/** A cell of the canvas is the same as the cell of a terminal's screen. 
		 */
		typedef vterm::Terminal::Cell Cell;

		/** Returns the width of the canvas in cells.
		 */
		int width() const {
			return width_;
		}

		/** Returns the height of the canvas in cells. 
		 */
		int height() const {
			return height_;
		}

		/** Fills the given rectangle of the canvas with given attributes. 
		 */
		void fill(Rect rect, Color bg, Color fg, Char fill, Font font);


		/** Displays the given text. 

		    TODO this is stupid api, should change
		 */
		void textOut(Point start, std::string const& text);

	private:
		friend class Widget;
		friend class Control;
		friend class RootWindow;
		friend class ScrollBox;

		/** Determins the visible region of the canvas.
		 */
		class VisibleRegion {
		public:

			/** Creates a visible region that encompases the entirety of a given root window. 
			 */
			VisibleRegion(RootWindow* root);

			/** Creates a new visible region within existing visible region. 
			 */
			VisibleRegion(VisibleRegion const& from, int left, int top, int width, int height);

			bool isValid() const {
				return root != nullptr;
			}

			void invalidate() {
				root = nullptr;
			}

			/** Returns the given point in canvas coordinates translated to the screen coordinates. 
			 */
			helpers::Point<unsigned> translate(Point what) {
				ASSERT(region.contains(what));
				return helpers::Point<unsigned>(
					static_cast<unsigned>(what.col - region.left + windowOffset.col),
					static_cast<unsigned>(what.row - region.top + windowOffset.row)
				);
			}

			/** Determines whether the visible region contains given screen column and row. 
			 */
			bool contains(unsigned col, unsigned row) {
				if (root == nullptr)
					return false;
				if (col < static_cast<unsigned>(windowOffset.col) || row < static_cast<unsigned>(windowOffset.row))
					return false;
				if (col >= static_cast<unsigned>(windowOffset.col) + region.width() || row >= static_cast<unsigned>(windowOffset.row + region.height()))
					return false;
				return true;
			}

			RootWindow* root;

			/** The visible region, in the coordinates of the canvas of the control.
			 */
			Rect region;

			/** The coordinates of the top left visible corner in the root window buffer.
			 */
			Point windowOffset;
		};

		/** Creates the canvas from given valid visible region, underlying screen and width & height. 
		 */
		Canvas(VisibleRegion const& from, vterm::Terminal::Screen& screen, int width, int height) :
			visibleRegion_(from),
			screen_(screen),
			width_(width),
		    height_(height) {
			ASSERT(from.isValid());
		}

		/** Creates a canvas into a rectangle in existing canvas.

		    Reuses the screen from the existing canvas and recalculates the visible region to reflect the child selected rectangle's position and dimension. 
		 */
		Canvas(Canvas const& from, int left, int top, int width, int height);

		/** Returns the cell at given canvas coordinates if visible, or nullptr if the cell is outside the visible region. 
		 */
		Cell* at(Point p) {
			// if the coordinates are not in visible region, return nullptr
			if (!visibleRegion_.region.contains(p))
				return nullptr;
			// otherwise recalculate the coordinates to the screen ones and return the cell
			return &screen_.at(visibleRegion_.translate(p));
		}

		VisibleRegion visibleRegion_;

		vterm::Terminal::Screen& screen_;

		int width_;
		int height_;

	}; // ui::Canvas

} // namespace ui
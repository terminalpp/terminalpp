#pragma once

#include <cstdint>

#include "helpers/helpers.h"
#include "helpers/locks.h"

#include "cell.h"
#include "shapes.h"

namespace ui {

    class Widget;
    class RootWindow;
    class Selection;


    /** Drawable surface of the UI. 
     
        The canvas represents provides an API To draw the UI elements. The canvas is represented by its backing buffer which is the actual array of cells that can be altered. 
     */
    class Canvas {
    public:

        /* Backing buffer. */
        class Buffer;

        /** Given existing canvas, creates a canvas encompassing the given subset of the original canvas. 
         */
        Canvas(Canvas & from, int left, int top, int width, int height);

        ~Canvas();

        int width() const {
            return width_;
        }

        int height() const {
            return height_;
        }

        /** Sets the cursor to gievn behavior and position. 
         
            Marks the cell at cursor's position to contain the cursor and updates the cursor definition in the root window to the provided cursor info and translated coordinates. Having the cursor position both in the cursor info and the cell itself has the benefit of quickly checking whether the cursor is visible or has been overriden by other contents.
         */
        void setCursor(Cursor const & cursor);

		/** Fills the given rectangle with specified brush. 

		    See the Brush class for more details. 
		 */
        void fill(Rect const & rect, Brush const & brush);

        /** Fills the given selection with specified brush. 
         */
        void fill(Selection const & sel, Brush const & brush);

		/** Displays the given text. 

		    TODO this is stupid api, should change
		 */
		void textOut(Point start, std::string const& text, Color color, Font font = Font());

        /** Copies the given buffer to specified coordinates. 
         
            The buffer must provide width(), height() and at() methods, alternatively the template can be manually specialized to a different type.
         */
        template<typename BUFFER>
        void copyBuffer(int x, int y, BUFFER const & buffer) {
            int xe = std::min(x + buffer.width(), width()) - x;
            int ye = std::min(y + buffer.height(), height()) - y;
            for (int by = 0; by < ye; ++by)
                for (int bx = 0; bx < xe; ++bx)
                    if (Cell * c = at(Point(x + bx, y + by))) 
                        *c = buffer.at(bx, by);
        }

    private:
        friend class Widget;
        friend class RootWindow;


        /** Determines the visible region of the canvas, i.e. the part of the canvas that is backed by the root window's buffer. 
         */
        class VisibleRegion {
        public:

            /** Link to the root window which contains the buffer. */
            RootWindow * root;

            /** Visible region of the canvas in canvas' coordinates.
              
                All coordinates of the region are expected to be positive. 
             */
            Rect region;

            /** Offset of the top-left corner of the region in root window's coordinates.
             */
            Point windowOffset;

            /** Determines whether the visible region is valid or not. 
             */
            bool valid;

			VisibleRegion() :
				root(nullptr),
				region(0, 0),
				windowOffset(0, 0),
                valid{false} {
			}

            VisibleRegion(RootWindow * root);

            VisibleRegion(VisibleRegion const & from, int left, int top, int width, int height);

			/** Returns the given point in canvas coordinates translated to the screen coordinates. 
			 */
			Point translate(Point what) {
				ASSERT(region.contains(what));
				return Point(
					(what.x - region.left() + windowOffset.x),
					(what.y - region.top() + windowOffset.y)
				);
			}

			/** Determines whether the visible region contains given screen column and row. 
			 */
			bool contains(unsigned col, unsigned row) {
				if (root == nullptr)
					return false;
				if (col < static_cast<unsigned>(windowOffset.x) || row < static_cast<unsigned>(windowOffset.y))
					return false;
				if (col >= static_cast<unsigned>(windowOffset.x) + region.width() || row >= static_cast<unsigned>(windowOffset.y + region.height()))
					return false;
				return true;
			}

        };

        Canvas(VisibleRegion const & visibleRegion, int width, int height);

        /** Fills given cell, if exists with given brush. 
         */
        void fill(Cell * cell, Brush const & brush);

		/** Returns the cell at given canvas coordinates if visible, or nullptr if the cell is outside the visible region. 
		 */
		Cell* at(Point p);

        /* Dimensions of the canvas. */
        int width_;
        int height_;

        VisibleRegion visibleRegion_;

        Canvas::Buffer & buffer_;

        static Canvas Create(Widget const & widget);
    };


    /** Canvas backing buffer. 
     
        Represents the screen that can be displayed and consists of a 2D mapped array of cells. 
    */
    class Canvas::Buffer {
    public:

        /** Smart pointer which automatically releases the lock on the buffer when destroyed.
         */
        typedef helpers::SmartRAIIPtr<Buffer> Ptr;

        Buffer(int cols, int rows):
            cols_(0),
            rows_(0),
            cells_(nullptr),
            lockDepth_(0) {
			resize(cols, rows);
        }

        /** Deleting the buffer deletes the underlying cell array.
         */
        ~Buffer() {
            delete [] cells_;
        }

        void resize(int cols, int rows) {
            if (cols_ != cols || rows_ != rows) {
                delete [] cells_;
                cols_ = cols;
                rows_ = rows;
                cells_ = new Cell[cols_ * rows_];
				// TODO delete this when done with randomized stuff
				for (int r = 0; r < rows; ++r)
					for (int c = 0; c < cols; ++c)
						at(c, r) << ((rand() % 64) + 32);
            }
        }

        int cols() const {
            return cols_;
        }

        int rows() const {
            return rows_;
        }

        Cell const & at(int x, int y) const {
            ASSERT(x >= 0 && x <= cols_ && y >= 0 && y < rows_);
            return cells_[y * cols_ + x];
        }

        Cell & at(int x, int y) {
            ASSERT(x >= 0 && x <= cols_ && y >= 0 && y < rows_);
            return cells_[y * cols_ + x];
        }

        Cell const & at(Point p) const {
            ASSERT(p.x >= 0 && p.x <= cols_ && p.y >= 0 && p.y < rows_);
            return cells_[p.y * cols_ + p.x];
        }

        Cell & at(Point p) {
            ASSERT(p.x >= 0 && p.x <= cols_ && p.y >= 0 && p.y < rows_);
            return cells_[p.y * cols_ + p.x];
        }

        void lock() {
            lock_.lock();
        }

        void priorityLock() {
            lock_.priorityLock();
        }

        void unlock() {
            lock_.unlock();
        }

    private:

        friend class Canvas;

        /* Size of the buffer and the array of the cells. */
        int cols_;
        int rows_;
        Cell * cells_;

        /* Depth of the locking done by the canvases. */
        unsigned lockDepth_;
        
        /* Priority lock on the buffer. */
        helpers::PriorityLock lock_;



    }; // Canvas::Buffer


    inline Cell * Canvas::at(Point p) {
        // if the coordinates are not in visible region, return nullptr
        if (!visibleRegion_.region.contains(p))
            return nullptr;
        // otherwise recalculate the coordinates to the screen ones and return the cell
        return &buffer_.at(visibleRegion_.translate(p));
    }



} // namespace ui
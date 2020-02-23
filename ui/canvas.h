#pragma once

#include <cstdint>

#include "helpers/helpers.h"
#include "helpers/locks.h"

#include "cell.h"
#include "shapes.h"

namespace ui {

    using helpers::Char;

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

        Canvas(Widget const * widget);

	    /** Copy constructor. 
		 
		    Increases the buffer's lock depth. 
		 */
		Canvas(Canvas const & from);

		/** Destructor. 
		 
		    If this is the last canvas to be destroyed for the given buffer, informs the root window that the visible rectangle should be updated. Otherwise just decreases the buffer's lock depth.
		 */
		~Canvas();

		/** Clips the canvas to a given rectangle. 
		 
		    Adjusts the visible rectangle accordingly.
		 */
	    Canvas & clip(Rect const & rect);

		/** \name Resizes the canvas to given width and height. 
		  
		    If the new size is smaller, the visible rectangle may be clipped, or even become empty.  
		 */
		//@{
		Canvas & resize(int width, int height);
		Canvas & resize(Point const & dimensions) {
			return resize(dimensions.x, dimensions.y);
		}
		//@}
		
		/** \name Moves the visible rectangle by the given offset. 
		  
		    If scrolled outside of the canvas itself, the visible rectangle is clipped.
		 */
		//@{
		Canvas & scrollBy(int x, int y) {
			return scrollBy(Point{x, y});
		}
		Canvas & scrollBy(Point const & offset);
		//@}

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

        /** Returns the rect encompassing the whole canvas. 
         */
        Rect rect() const {
            return Rect::FromWH(width(), height());
        }

        //@{

        Color fg() const {
            return fg_;
        }    

        Brush const & bg() const {
            return bg_;
        }

        Color decorationColor() const {
            return decorationColor_;
        }

        Font font() const {
            return font_;
        }

        Attributes attributes() const {
            return attrs_;
        }

        Canvas & setFg(Color const & value) {
            fg_ = value;
            return *this;
        }

        Canvas & setBg(Brush const & value) {
            bg_ = value;
            return *this;
        }

        Canvas & setDecorationColor(Color const & value) {
            decorationColor_ = value;
            return *this;
        }

        Canvas & setFont(Font const & value) {
            font_ = value;
            return *this;
        }

        Canvas & setAttributes(Attributes const & value) {
            attrs_ = value;
            return *this;
        }

        //@}

		/** Fills the given rectangle with specified brush. 

		    See the Brush class for more details. 
		 */
        void fill(Rect const & rect, Brush const & brush);

        void fill(Rect const & rect) {
            fill(rect, bg_);
        }

        /** Fills the given selection with specified brush. 
         */
        void fill(Selection const & sel, Brush const & brush);

        void fill(Selection const & sel) {
            fill(sel, bg_);
        }

        /** Sets the cell at given coordinates to the given template. 
         */
        void set(Point const & p, Cell const & cell) {
            Cell * c = at(p);
            if (c != nullptr)
                *c = cell;
        }

        /** Draws a single line of text at the top of the given rectange. 
         
            Any overflow is clipped, supports the UTF8 encoding in the text properly. 
         */

        void lineOut(Rect const & rect, Char::iterator_utf8 start, Char::iterator_utf8 end, HorizontalAlign halign = HorizontalAlign::Left);

        void lineOut(Rect const & rect, std::string const & text, HorizontalAlign halign = HorizontalAlign::Left) {
            lineOut(rect, Char::BeginOf(text), Char::EndOf(text), halign);
        }

        void lineOut(Point const & from, std::string const & text, HorizontalAlign halign = HorizontalAlign::Left) {
            lineOut(Rect::FromTopLeftWH(from, width(), 1), Char::BeginOf(text), Char::EndOf(text), halign);
        }

        /** Outputs the given text line by line in the provided rectangle. 
         
            If wordWrap is true (default), lines that are too long will be broken up at word boundaries. 
         */
        void textOut(Rect const & rect, std::string const & text, HorizontalAlign halign = HorizontalAlign::Left, bool wordWrap = true);

        /** Clears the given rectangle of any visible borders. 
         
            Disables the top, left, bottom and right borders, but leaves border color and thickness unchanged. 
         */
        void clearBorder(Rect const & rect);

        /** Draws border around the specified rectangle with given color & thickness. 
         */
        void borderRect(Rect const & rect, Border const & border);

        void borderLineTop(Point start, int width, Color color, Border::Kind kind);
        void borderLineBottom(Point start, int width, Color color, Border::Kind kind);

        void borderLineLeft(Point start, int height, Color color, Border::Kind kind);
        void borderLineRight(Point start, int height, Color color, Border::Kind kind);

        void drawRightVerticalScrollBar(Point from, int size, int sliderStart, int sliderSize, Color color, bool thick = false);

        /** Copies the given buffer to specified coordinates. 
         
            The buffer must provide width(), height() and at() methods, alternatively the template can be manually specialized to a different type.
         */
        template<typename BUFFER>
        void copyBuffer(int x, int y, BUFFER const & buffer) {
            int xe = std::min(x + buffer.width(), width()) - x;
            int ye = std::min(y + buffer.height(), height()) - y;
            for (int by = 0; by < ye; ++by) {
                if (at(Point(x, y + by))) {
                    for (int bx = 0; bx < xe; ++bx)
                        if (Cell * c = at(Point(x + bx, y + by))) 
                            *c = buffer.at(bx, by);
                }
            }
        }

        /** Determines the height required for the gievn text to be displayed. 
         */ 
        static int TextHeight(Char::iterator_utf8 begin, Char::iterator_utf8 end, Font font, int maxWidth, bool wordWrap = true);

        static int TextHeight(std::string const & text, Font font, int maxWidth, bool wordWrap = true) {
            return TextHeight(Char::BeginOf(text), Char::EndOf(text), font, maxWidth, wordWrap);
        }

    private:
        friend class Widget;
        friend class Container;
        friend class RootWindow;

        friend class TestCanvasAccessor;

        /** Description of the visible rectangle of a canvas. 
		 */
		class VisibleRect {
		public:

			VisibleRect():
			    rootWindow_{nullptr},
				valid_{false},
				rect_{Rect::Empty()} {
			}

			VisibleRect(Rect const & rect, Point const & bufferOffset, RootWindow * root):
			    rootWindow_(root),
				valid_(true),
				rect_(rect),
				bufferOffset_(bufferOffset) {
			}

		    bool empty() const {
				ASSERT(valid_);
				return rect_.width() == 0 || rect_.height() == 0;
			}

			bool valid() const {
				return valid_;
			}

            // TODO this should die too I think. 
            // takes root window's coordinates and converts them to widget's coordinates 
            void toWidgetCoordinates(int & col, int & row) const {
                col = col - bufferOffset_.x + rect_.left();
                row = row - bufferOffset_.y + rect_.top();
            }

			/** Invalidates the visible rectangle. 
			 
			    A canvas cannot be created from an invalid rectangle. 
			 */
			void invalidate() {
				valid_ = false;
			}

            void attach(RootWindow * root) {
                ASSERT(! valid_);
                rootWindow_ = root;
            }

			/** Detaches the rectangle.
			 
			    Invalidates the rectangle and clears the root window information. 
			 */
			void detach() {
				valid_ = false;
				rootWindow_ = nullptr;
				rect_ = Rect::Empty();
			}

			RootWindow * rootWindow() const {
				return rootWindow_;
			}

			/** Returns the rectangle in root window coordinates. 
			 
			    These are the direct indices to the buffer. 
			 */
			Rect rootWindowRectangle() const {
				return rect_ + bufferOffset_;
			}

			bool operator == (VisibleRect const & other) const {
				return rootWindow_ == other.rootWindow_ && rect_ == other.rect_ && bufferOffset_ == other.bufferOffset_ && valid_ == other.valid_;
			}

			bool operator != (VisibleRect const & other) const {
				return rootWindow_ != other.rootWindow_ || rect_ != other.rect_ || bufferOffset_ != other.bufferOffset_ || valid_ != other.valid_;
			}

		private:
		    friend class Canvas;
			friend class TestCanvasAccessor;

		    RootWindow * rootWindow_;
			bool valid_;

            /** Visible region of the canvas in canvas' coordinates.
              
                All coordinates of the region are expected to be positive. 
             */
			Rect rect_;

            /** Offset of the top-left corner of the region in root window's coordinates.
             */
			Point bufferOffset_;
		};

        /** Draws a single line of text, left justified.
            
            Returns false if the drawn length was smaller or equal to the maxLength argument, true otherwise (i.e. whet clipping occured).
         */
        bool lineOutLeft(Rect const & rect, Char::iterator_utf8 begin, Char::iterator_utf8 end);
        bool lineOutRight(Rect const & rect, Char::iterator_utf8 begin, Char::iterator_utf8 end);
        bool lineOutCenter(Rect const & rect, Char::iterator_utf8 begin, Char::iterator_utf8 end);

        /** Fills given cell, if exists with given brush. 
         */
        void fill(Cell * cell, Brush const & brush);

		/** Returns the cell at given canvas coordinates if visible, or nullptr if the cell is outside the visible region. 
		 */
		Cell* at(Point p);

        /* Dimensions of the canvas. */
        int width_;
        int height_;

        VisibleRect visibleRect_;

        Canvas::Buffer & buffer_;

        /* Current foreground color.  */
        Color fg_;
        /* Current background brush. */
        Brush bg_;
        /* Color for decorations (underline, strikethrough, etc.) */
        Color decorationColor_;
        /* Current font. */
        Font font_;
        /* Current cell attributes. */
        Attributes attrs_;
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
        if (visibleRect_.valid() == false || ! visibleRect_.rect_.contains(p))
            return nullptr;
        // otherwise recalculate the coordinates to the screen ones and return the cell
        return &buffer_.at(p - visibleRect_.rect_.topLeft() + visibleRect_.bufferOffset_);		
	}

} // namespace ui
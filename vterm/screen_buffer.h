#pragma once


#include "helpers/helpers.h"

#include "color.h"
#include "char.h"
#include "font.h"

namespace vterm {

	/** Screen buffer containing the information about the contents to be displayed.
	 */
	class ScreenBuffer {
	public:

		/** Contains information about single display cell. 

		    Although quite a lot memory is required for each cell, this should be perfectly fine since we need only a very small ammount of cells for any terminal window.
		 */
		class Cell {
		public:
			/** Foreground - text color. 
			 */
			Color fg;

			/** Background color. 
			 */
			Color bg;

			/** The character in the cell. 
			 */
			Char c;

			/** The font to be used for displaying the cell. 
			 */
			Font font;

		}; // ScreenBuffer::Cell

		/** Returns the number of the rows of the screenbuffer. 
		 */
		size_t rows() const {
			return rows_;
		}

		/** Returns the number of columns of the screenbuffer. 
		 */
		size_t cols() const {
			return cols_;
		}

		/** Returns the appropriate cell.
		 */
		Cell const & at(size_t col, size_t row) const {
			ASSERT(col < cols_ && row < rows_) << "Indices " << col << ";" << row << " out of bounds " << cols_ << ";" << rows_;
			return cells_[row * cols_ + col];
		}
		/** Returns the appropriate cell.
		 */
		Cell & at(size_t col, size_t row) {
			ASSERT(col < cols_ && row < rows_) << "Indices " << col << ";" << row << " out of bounds " << cols_ << ";" << rows_;
			return cells_[row * cols_ + col];
		}

		/** Resizes the virtual terminal. 

		    TODO check concurrency and synchronization of this so that resize does not clash with the writers to the buffer
		 */
		void resize(size_t cols, size_t rows) {
			if (cols == cols_ && rows_ == rows_)
				return;
			// delete old cells
			delete [] cells_;
			cols_ = cols;
			rows_ = rows;
			// create new cells
			cells_ = new Cell[cols * rows];
		}

	private:
		size_t rows_;
		size_t cols_;

		Cell * cells_ = nullptr;

	}; // vterm::ScreenBuffer

} // namespace vterm
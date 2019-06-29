
#include <cstring>

#include "helpers/log.h"
#include "helpers/strings.h"

#include "terminal.h"
#include "pty.h"

namespace vterm {

	/*
	void Terminal::PTYBackend::resizeComBuffer(size_t newSize) {
		ASSERT(newSize >= static_cast<size_t>(writeStart_ - buffer_)) << "Not enough space for unprocessed data after resizing (unprocessed " << (writeStart_ - buffer_) << ", requested size " << newSize << ")";
		char* nb = new char[newSize];
		memcpy(nb, buffer_, writeStart_ - buffer_);
		writeStart_ = nb + (writeStart_ - buffer_);
		delete [] buffer_;
		buffer_ = nb;
	} 
	*/

	std::string Terminal::getText(Selection const & selection) const	{
		std::stringstream result;
		unsigned col = selection.start.col;
		unsigned row = selection.start.row;
		// TODO this is ugly
		ScreenLock sl = const_cast<Terminal*>(this)->lockScreen();
		while (selection.contains(col, row) && col < sl->cols() && row < sl->rows()) {
			result << sl->at(col, row).c;
			if (++col == sl->cols()) {
				++row;
				col = 0;
				result << '\n';
			}
		}
		return result.str();
	}

} // namespace vterm
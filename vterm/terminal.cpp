
#include <cstring>

#include "helpers/log.h"

#include "terminal.h"
#include "pty.h"

namespace vterm {

	std::string Terminal::getText(Selection const & selection) const	{
		std::stringstream result;
		unsigned col = selection.start.col;
		unsigned row = selection.start.row;
		// TODO this is ugly
		ScreenLock sl = const_cast<Terminal*>(this)->lockScreen();
		while (selection.contains(col, row) && col < sl->cols() && row < sl->rows()) {
			result << sl->at(col, row).c();
			if (++col == sl->cols()) {
				++row;
				col = 0;
				result << '\n';
			}
		}
		return result.str();
	}

} // namespace vterm
#pragma once
#ifdef WIN32

#include "vterm/vt100.h"
#include "vterm/win32/conpty_terminal.h"

namespace tpp {

    // Disable the inheritance via dominance warning as this is precisely what the terminal uses
    #pragma warning(disable:4250)

	class Terminal : public vterm::VT100, public vterm::ConPTYTerminal {
	public:
		Terminal(std::string const & cmd, unsigned cols, unsigned rows, vterm::Palette const & palette, unsigned defaultFg, unsigned defaultBg) :
			VT100(cols, rows, palette, defaultFg, defaultBg),
			ConPTYTerminal(cmd, cols, rows),
			IOTerminal(cols, rows) {
		}
	}; // tpp::Terminal
	#pragma warning(default:4250)


} // namespace tpp


#endif
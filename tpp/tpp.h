#pragma once 

#include "vterm/vt100.h"
#ifdef WIN32
#include "vterm/win32/conpty_terminal.h"
#endif

namespace tpp {

#ifdef WIN32
	typedef vterm::ConPTYTerminal PTYTerminal;
#endif


	class Terminal : public vterm::VT100, public PTYTerminal {
	public:
		Terminal(std::string const & cmd, unsigned cols, unsigned rows):
			VT100(cols, rows),
			PTYTerminal(cmd, cols, rows),
			vterm::IOTerminal(cols, rows) {
		}
	};






} // namespace tpp

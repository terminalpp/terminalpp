#pragma once
#ifdef WIN32__

#include "vterm/vt100.h"
//#include "vterm/win32/conpty_terminal.h"

namespace tpp {

    // Disable the inheritance via dominance warning as this is precisely what the terminal uses
    #pragma warning(disable:4250)

	class Terminal : public vterm::VT100, public vterm::ConPTYTermin {
	public:
		Terminal(std::string const & cmd, unsigned cols, unsigned rows, vterm::Palette const & palette, unsigned defaultFg, unsigned defaultBg) :
			IOTerminal(cols, rows),
			VT100(cols, rows, palette, defaultFg, defaultBg),
			ConPTYTerminal(cmd, cols, rows) {
		}

	protected:

		void resize(unsigned cols, unsigned rows) override {
			IOTerminal::doResize(cols, rows);
			VT100::doResize(cols, rows);
			ConPTYTerminal::doResize(cols, rows);
		}

	}; // tpp::Terminal
	#pragma warning(default:4250)


} // namespace tpp


#endif
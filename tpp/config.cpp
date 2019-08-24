#include "config.h"

namespace tpp { namespace config {

#ifdef ARCH_WINDOWS
		helpers::Arg<bool> UseConPTY(
			{ "--use-conpty" },
			false,
			false,
			"Uses the Win32 ConPTY pseudoterminal instead of the WSL bypass"
		);
#endif
		helpers::Arg<unsigned> FPS(
			{ "--fps" },
			DEFAULT_TERMINAL_FPS,
			false,
			"Maximum number of fps the terminal will display"
		);
		helpers::Arg<unsigned> Cols(
			{ "--cols", "-c" },
			DEFAULT_TERMINAL_COLS,
			false,
			"Number of columns of the terminal window"
		);
		helpers::Arg<unsigned> Rows(
			{ "--rows", "-r" },
			DEFAULT_TERMINAL_ROWS,
			false,
			"Number of rows of the terminal window"
		);
		helpers::Arg<std::string> FontFamily(
			{ "--font" }, 
			DEFAULT_TERMINAL_FONT_FAMILY, 
			false, 
			"Font to render the terminal with"
		);
		helpers::Arg<unsigned> FontSize(
			{ "--font-size" }, 
			DEFAULT_TERMINAL_FONT_SIZE,
			false,
			"Size of the font in pixels at no zoom."
		);
		helpers::Arg<std::vector<std::string>> Command(
			{ "-e" }, 
			{ DEFAULT_TERMINAL_COMMAND },
			false,
			"Determines the command to be executed in the terminal",
			true
		);


}} // namespace tpp::config
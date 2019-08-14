#pragma once
#include "helpers/args.h"

#define SHOW_LINE_ENDINGS


/** Determines the default blink speed of the cursor or blinking text. 
 */ 
#define DEFAULT_BLINK_SPEED 500


/** Keyboard shortcuts for various actions.
 */
#define SHORTCUT_ZOOM_IN (ui::Key::Equals + ui::Key::Ctrl)
#define SHORTCUT_ZOOM_OUT (ui::Key::Minus + ui::Key::Ctrl)
#define SHORTCUT_FULLSCREEN (ui::Key::Enter + ui::Key::Alt)
#define SHORTCUT_PASTE (ui::Key::V + ui::Key::Ctrl + ui::Key::Shift)


namespace tpp {
	namespace config {
		/** Maximum number of frames per second the renderer is allowed to render.

			There is very little point in setting this higher than the screen's v-sync rate. 30 fps seems to be just fine, 60 fps means more updated and greater stress on the terminal.
		 */
		extern helpers::Arg<unsigned> FPS;
        #define DEFAULT_TERMINAL_FPS 60

		/** Initial size of the terminal window and the underlying terminal in text columns an rows.
		 */
		extern helpers::Arg<unsigned> Cols;
		extern helpers::Arg<unsigned> Rows;
		#define DEFAULT_TERMINAL_COLS 80
		#define DEFAULT_TERMINAL_ROWS 25

		/** Name of the font to be used and its size in pixels for zoom 1.
		 */
		extern helpers::Arg<std::string> FontFamily;
		extern helpers::Arg<unsigned> FontSize;
		#define DEFAULT_TERMINAL_FONT_FAMILY "Iosevka Term"
		#define DEFAULT_TERMINAL_FONT_SIZE 18

		/** The command to be executed.

			This is usually the shell. Depends on the OS used and pseudoterminal interface.

			This particular command starts the bypass in wsl, overrides the SHELL envvar to bash (default) and then instructs the bypass to run bash.
		 */
		extern helpers::Arg<std::vector<std::string>> Command;
#ifdef ARCH_WINDOWS
		#define DEFAULT_TERMINAL_COMMAND "wsl", "-e", "/home/peta/devel/tpp-build/bypass/bypass", "SHELL=/bin/bash", "-e", "bash"
#else
        #define DEFAULT_TERMINAL_COMMAND "bash"
#endif

#ifdef ARCH_WINDOWS
		/** Specifies the pseudoterminal interface to be used.

			The default pseudoterminal on the given platform is `vterm::LocalPTY`. However on windows this terminal performs its own destructive operations on the I/O. A `BypassPTY` was created, which in a simple program running in the wsl which translates the terminal communication and events over simple process I/O, thus bypassing the system pseudoterminal.
		 */
		extern helpers::Arg<bool> UseConPTY;
#endif

		/** Optional argument that specifies file in which all incomming data will be stored. 

		    This essentially logs all the text and escape sequences that were sent to the terminal during a session. 
		 */
		extern helpers::Arg<std::string> RecordSession;

	} // namespace config
} // namespace tpp
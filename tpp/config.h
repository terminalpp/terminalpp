/** Maximum number of frames per second the renderer is allowed to render. 

    There is very little point in setting this higher than the screen's v-sync rate. 30 fps seems to be just fine, 60 fps means more updated and greater stress on the terminal. 
 */
#define DEFAULT_FPS 60

/** Initial size of the terminal window and the underlying terminal in text columns an rows. 
 */
#define DEFAULT_TERMINAL_COLS 80
#define DEFAULT_TERMINAL_ROWS 25

/** Name of the font to be used and its size in pixels for zoom 1. 
 */
#define DEFAULT_TERMINAL_FONT "Iosevka Term"
#define DEFAULT_TERMINAL_FONT_SIZE 13

/** Keyboard shortcuts for various actions. 
 */
#define SHORTCUT_ZOOM_IN (vterm::Key::Equals + vterm::Key::Ctrl)
#define SHORTCUT_ZOOM_OUT (vterm::Key::Minus + vterm::Key::Ctrl)
#define SHORTCUT_FULLSCREEN (vterm::Key::Enter + vterm::Key::Alt)
#define SHORTCUT_PASTE (vterm::Key::V + vterm::Key::Ctrl + vterm::Key::Shift)


/** Configuration for the supported systems.

    Three things should be specified for each system, namely the terminal, pty and command. Their decription can be found in their Windows definitions. 
 */

#ifdef _WIN64

/** Specifies what kind of terminal should be used. Currently only VT100 is supported. 
 */
#define DEFAULT_SESSION_TERMINAL vterm::VT100

/** Specifies the pseudoterminal interface to be used. 

    The default pseudoterminal on the given platform is `vterm::LocalPTY`. However on windows this terminal performs its own destructive operations on the I/O. A `BypassPTY` was created, which in a simple program running in the wsl which translates the terminal communication and events over simple process I/O, thus bypassing the system pseudoterminal.  
 */
#define DEFAULT_SESSION_PTY vterm::BypassPTY

/** The command to be executed. 

    This is usually the shell. Depends on the OS used and pseudoterminal interface. 

	This particular command starts the bypass in wsl, overrides the SHELL envvar to bash (default) and then instructs the bypass to run bash. 
 */
#define DEFAULT_SESSION_COMMAND helpers::Command("wsl", {"-e", "/home/peta/devel/tpp-build/bypass/bypass", "-env", "SHELL=/bin/bash", "--", "bash"})


/* Use these definitions in you want to use the Windows native pseudoterminal implementation instead. 

   This is useful for running things like powershell, etc. 
*/
//#define DEFAULT_SESSION_PTY vterm::LocalPTY
//#define DEFAULT_SESSION_COMMAND helpers::Command("wsl", {"-e", "bash"})

#elif __linux__

#define DEFAULT_SESSION_TERMINAL vterm::VT100
#define DEFAULT_SESSION_PTY vterm::LocalPTY
#define DEFAULT_SESSION_COMMAND helpers::Command("bash", {})

#endif
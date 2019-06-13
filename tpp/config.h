#define DEFAULT_TERMINAL_COLS 80
#define DEFAULT_TERMINAL_ROWS 25
#define DEFAULT_TERMINAL_FONT_HEIGHT 18

#ifdef WIN32

#define DEFAULT_SESSION_COMMAND helpers::Command("wsl", {"-e", "bash"})

#elif __linux__

//#define DEFAULT_SESSION_COMMAND helpers::Command("bash", {})
#define DEFAULT_SESSION_COMMAND helpers::Command("/home/peta/devel/terminalpp/build/asciienc/asciienc", {"emacs", "-nw"})

#endif
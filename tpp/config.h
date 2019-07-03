#define DEFAULT_FPS 60
#define DEFAULT_TERMINAL_COLS 80
#define DEFAULT_TERMINAL_ROWS 25
#define DEFAULT_TERMINAL_FONT_SIZE 12

#ifdef _WIN64
#define DEFAULT_TERMINAL_FONT L"Iosevka Term"

#define DEFAULT_SESSION_BACKEND vterm::VT100


#define DEFAULT_SESSION_PTY vterm::BypassPTY
//#define DEFAULT_SESSION_PTY vterm::LocalPTY



//#define DEFAULT_SESSION_COMMAND helpers::Command("powershell", {})
#define DEFAULT_SESSION_COMMAND helpers::Command("wsl", {"-e", "/home/peta/devel/tpp-build/asciienc/asciienc", "-env", "SHELL=/bin/bash", "--", "bash"})
//#define DEFAULT_SESSION_COMMAND helpers::Command("wsl", {"-e", "bash"})

#elif __linux__
#define DEFAULT_TERMINAL_FONT "Iosevka Term"

#define DEFAULT_SESSION_BACKEND vterm::VT100
#define DEFAULT_SESSION_PTY vterm::LocalPTY

#define DEFAULT_SESSION_COMMAND helpers::Command("bash", {})
//#define DEFAULT_SESSION_COMMAND helpers::Command("/home/peta/devel/tpp-build/asciienc/asciienc", {"mc"})

#endif
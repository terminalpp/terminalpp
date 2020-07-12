#ifndef KEY // lgtm [cpp/missing-header-guard]
#error "KEY macro must be defined before including ansi_keys.inc.h"
#endif

#define VT_MODIFIERS(K, SEQ1, SEQ2) KEY(K + Key::Shift, SEQ1 << 2 << SEQ2); \
                                    KEY(K + Key::Alt, SEQ1 << 3 << SEQ2); \
                                    KEY(K + Key::Shift + Key::Alt, SEQ1 << 4 << SEQ2); \
                                    KEY(K + Key::Ctrl, SEQ1 << 5 << SEQ2); \
                                    KEY(K + Key::Ctrl + Key::Shift, SEQ1 << 6 << SEQ2); \
                                    KEY(K + Key::Ctrl + Key::Alt, SEQ1 << 7 << SEQ2); \
                                    KEY(K + Key::Ctrl + Key::Alt + Key::Shift, SEQ1 << 8 << SEQ2);

#define VT_KEY(K) KEY(K, static_cast<char>(K.code() + 32)); \
                  KEY(K + Key::Shift, static_cast<char>(K.code())); \
                  KEY(K + Key::Ctrl, static_cast<char<(K.code() + 1 - 'A')); \
                  KEY(K + Key::Ctrl + Key::Shift, static_cast<char<(K.code() + 1 - 'A')); \
                  KEY(K + Key::Alt, '\033' << static_cast<char>(K.code() + 32)); \
                  KEY(K + Key::Alt + Key::Shift, '\033' << static_cast<char>(K.code())); \
                  KEY(K + Key::Alt + Key::Ctrl, '\033' << static_cast<char<(K.code() + 1 - 'A')); \
                  KEY(K + Key::Alt + Key::Ctrl + '\033' << Key::Shift, static_cast<char<(K.code() + 1 - 'A'));

#define VT_NUM(K) KEY(K, static_cast<char>(K.code())); \
                  KEY(K, '\033' << static_cast<char>(K.code()));

// letters and their modifiers          
VT_KEY(Key::A)
VT_KEY(Key::B)
VT_KEY(Key::C)
VT_KEY(Key::D)
VT_KEY(Key::E)
VT_KEY(Key::F)
VT_KEY(Key::G)
VT_KEY(Key::H)
VT_KEY(Key::I)
VT_KEY(Key::J)
VT_KEY(Key::K)
VT_KEY(Key::L)
VT_KEY(Key::M)
VT_KEY(Key::N)
VT_KEY(Key::O)
VT_KEY(Key::P)
VT_KEY(Key::Q)
VT_KEY(Key::R)
VT_KEY(Key::S)
VT_KEY(Key::T)
VT_KEY(Key::U)
VT_KEY(Key::V)
VT_KEY(Key::W)
VT_KEY(Key::X)
VT_KEY(Key::Y)
VT_KEY(Key::Z)

// numbers and modifiers 
VT_NUM(Key::Num0)
VT_NUM(Key::Num1)
VT_NUM(Key::Num2)
VT_NUM(Key::Num3)
VT_NUM(Key::Num4)
VT_NUM(Key::Num5)
VT_NUM(Key::Num6)
VT_NUM(Key::Num7)
VT_NUM(Key::Num8)
VT_NUM(Key::Num9)

// ctrl + 2 is 0
KEY(Key::Num0 + Key::Ctrl, "\000");
// alt + shift keys and other extre keys
KEY(Key::Num0 + Key::Shift + Key::Alt, "\033)");
KEY(Key::Num1 + Key::Shift + Key::Alt, "\033!");
KEY(Key::Num2 + Key::Shift + Key::Alt, "\033@");
KEY(Key::Num3 + Key::Shift + Key::Alt, "\033#");
KEY(Key::Num4 + Key::Shift + Key::Alt, "\033$");
KEY(Key::Num5 + Key::Shift + Key::Alt, "\033%");
KEY(Key::Num6 + Key::Shift + Key::Alt, "\033^");
KEY(Key::Num7 + Key::Shift + Key::Alt, "\033&");
KEY(Key::Num8 + Key::Shift + Key::Alt, "\033*");
KEY(Key::Num9 + Key::Shift + Key::Alt, "\033(");
// other special characters with alt
KEY(Key::Tick + Key::Alt, "\033`");
KEY(Key::Tick + Key::Shift + Key::Alt, "\033~");
KEY(Key::Minus + Key::Alt, "\033-");
KEY(Key::Minus + Key::Alt + Key::Shift, "\033_");
KEY(Key::Equals + Key::Alt, "\033=");
KEY(Key::Equals + Key::Alt + Key::Shift, "\033+");
KEY(Key::SquareOpen + Key::Alt, "\033[");
KEY(Key::SquareOpen + Key::Alt + Key::Shift, "\033{");
KEY(Key::SquareClose + Key::Alt, "\033]");
KEY(Key::SquareClose + Key::Alt + Key::Shift, "\033}");
KEY(Key::Backslash + Key::Alt, "\033\\");
KEY(Key::Backslash + Key::Alt + Key::Shift, "\033|");
KEY(Key::Semicolon + Key::Alt, "\033;");
KEY(Key::Semicolon + Key::Alt + Key::Shift, "\033:");
KEY(Key::Quote + Key::Alt, "\033'");
KEY(Key::Quote + Key::Alt + Key::Shift, "\033\"");
KEY(Key::Comma + Key::Alt, "\033,");
KEY(Key::Comma + Key::Alt + Key::Shift, "\033<");
KEY(Key::Dot + Key::Alt, "\033.");
KEY(Key::Dot + Key::Alt + Key::Shift, "\033>");
KEY(Key::Slash + Key::Alt, "\033/");
KEY(Key::Slash + Key::Alt + Key::Shift, "\033?");
// arrows, fn keys & friends
KEY(Key::Up, "\033[A");
KEY(Key::Down, "\033[B");
KEY(Key::Right, "\033[C");
KEY(Key::Left, "\033[D");
KEY(Key::Home, "\033[H"); // also \033[1~
KEY(Key::End, "\033[F"); // also \033[4~
KEY(Key::PageUp, "\033[5~");
KEY(Key::PageDown, "\033[6~");
KEY(Key::Insert, "\033[2~");
KEY(Key::Delete, "\033[3~");
KEY(Key::F1, "\033OP");
KEY(Key::F2, "\033OQ");
KEY(Key::F3, "\033OR");
KEY(Key::F4, "\033OS");
KEY(Key::F5, "\033[15~");
KEY(Key::F6, "\033[17~");
KEY(Key::F7, "\033[18~");
KEY(Key::F8, "\033[19~");
KEY(Key::F9, "\033[20~");
KEY(Key::F10, "\033[21~");
KEY(Key::F11, "\033[23~");
KEY(Key::F12, "\033[24~");

KEY(Key::Enter, "\r"); // carriage return, not LF
KEY(Key::Tab, "\t");
KEY(Key::Esc, "\033");
KEY(Key::Backspace, "\x7f");

VT_MODIFIERS(Key::Up, "\033[1;", "A");
VT_MODIFIERS(Key::Down, "\033[1;", "B");
VT_MODIFIERS(Key::Left, "\033[1;", "D");
VT_MODIFIERS(Key::Right, "\033[1;", "C");
VT_MODIFIERS(Key::Home, "\033[1;", "H");
VT_MODIFIERS(Key::End, "\033[1;", "F");
VT_MODIFIERS(Key::PageUp, "\033[5;", "~");
VT_MODIFIERS(Key::PageDown, "\033[6;", "~");

VT_MODIFIERS(Key::F1, "\033[1;", "P");
VT_MODIFIERS(Key::F2, "\033[1;", "Q");
VT_MODIFIERS(Key::F3, "\033[1;", "R");
VT_MODIFIERS(Key::F4, "\033[1;", "S");
VT_MODIFIERS(Key::F5, "\033[15;", "~");
VT_MODIFIERS(Key::F6, "\033[17;", "~");
VT_MODIFIERS(Key::F7, "\033[18;", "~");
VT_MODIFIERS(Key::F8, "\033[19;", "~");
VT_MODIFIERS(Key::F9, "\033[20;", "~");
VT_MODIFIERS(Key::F10, "\033[21;", "~");
VT_MODIFIERS(Key::F11, "\033[23;", "~");
VT_MODIFIERS(Key::F12, "\033[24;", "~");

KEY(Key::SquareOpen + Key::Ctrl, "\033");
KEY(Key::Backslash + Key::Ctrl, "\034");
KEY(Key::SquareClose + Key::Ctrl, "\035");

#undef VT_MODIFIERS
#undef VT_KEY
#undef VT_NUM
#undef KEY
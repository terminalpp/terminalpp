
#include "helpers/log.h"

#include "vt100.h"



namespace vterm {

    namespace {

        std::unordered_map<ui::Key, std::string> InitializeVT100KeyMap() {
            #define KEY(K, ...) ASSERT(km.find(K) == km.end()) << "Key " << K << " altrady defined"; km.insert(std::make_pair(K, STR(__VA_ARGS__)))
            #define VT_MODIFIERS(K, SEQ1, SEQ2) KEY(K + Key::Shift, SEQ1 << 2 << SEQ2); \
                                                KEY(K + Key::Alt, SEQ1 << 3 << SEQ2); \
                                                KEY(K + Key::Shift + Key::Alt, SEQ1 << 4 << SEQ2); \
                                                KEY(K + Key::Ctrl, SEQ1 << 5 << SEQ2); \
                                                KEY(K + Key::Ctrl + Key::Shift, SEQ1 << 6 << SEQ2); \
                                                KEY(K + Key::Ctrl + Key::Alt, SEQ1 << 7 << SEQ2); \
                                                KEY(K + Key::Ctrl + Key::Alt + Key::Shift, SEQ1 << 8 << SEQ2);

            using namespace ui;

            std::unordered_map<ui::Key, std::string> km;
            // first add letter keys in their modifications
            for (unsigned k = 'A'; k <= 'Z'; ++k) {
                // ctrl + letter and ctrl + shift + letter are the same
                KEY(Key(k) + Key::Ctrl, static_cast<char>(k + 1 - 'A'));
                KEY(Key(k) + Key::Ctrl + Key::Shift, static_cast<char>(k + 1 - 'A'));
                // alt simply prepends escape to whatever the non-alt key would be
                KEY(Key(k) + Key::Alt, '\033' << static_cast<char>(k + 32));
                KEY(Key(k) + Key::Shift + Key::Alt, '\033' << static_cast<char>(k));
                KEY(Key(k) + Key::Ctrl + Key::Alt, '\033' << static_cast<char>(k + 1 - 'A'));
                KEY(Key(k) + Key::Ctrl + Key::Shift + Key::Alt, '\033' << static_cast<char>(k + 1 - 'A'));
            }

            // modifiers + numbers
            for (unsigned k = '0'; k <= '9'; ++k) {
                // alt + key prepends escape character
                KEY(Key(k) + Key::Alt, '\033' << static_cast<char>(k));
            }

            // ctrl + 2 is 0
            KEY(Key(Key::Num0) + Key::Ctrl, "\000");
            // alt + shift keys and other extre keys
            KEY(Key(Key::Num0) + Key::Shift + Key::Alt, "\033)");
            KEY(Key(Key::Num1) + Key::Shift + Key::Alt, "\033!");
            KEY(Key(Key::Num2) + Key::Shift + Key::Alt, "\033@");
            KEY(Key(Key::Num3) + Key::Shift + Key::Alt, "\033#");
            KEY(Key(Key::Num4) + Key::Shift + Key::Alt, "\033$");
            KEY(Key(Key::Num5) + Key::Shift + Key::Alt, "\033%");
            KEY(Key(Key::Num6) + Key::Shift + Key::Alt, "\033^");
            KEY(Key(Key::Num7) + Key::Shift + Key::Alt, "\033&");
            KEY(Key(Key::Num8) + Key::Shift + Key::Alt, "\033*");
            KEY(Key(Key::Num9) + Key::Shift + Key::Alt, "\033(");
            // other special characters with alt
            KEY(Key(Key::Tick) + Key::Alt, "\033`");
            KEY(Key(Key::Tick) + Key::Shift + Key::Alt, "\033~");
            KEY(Key(Key::Minus) + Key::Alt, "\033-");
            KEY(Key(Key::Minus) + Key::Alt + Key::Shift, "\033_");
            KEY(Key(Key::Equals) + Key::Alt, "\033=");
            KEY(Key(Key::Equals) + Key::Alt + Key::Shift, "\033+");
            KEY(Key(Key::SquareOpen) + Key::Alt, "\033[");
            KEY(Key(Key::SquareOpen) + Key::Alt + Key::Shift, "\033{");
            KEY(Key(Key::SquareClose) + Key::Alt, "\033]");
            KEY(Key(Key::SquareClose) + Key::Alt + Key::Shift, "\033}");
            KEY(Key(Key::Backslash) + Key::Alt, "\033\\");
            KEY(Key(Key::Backslash) + Key::Alt + Key::Shift, "\033|");
            KEY(Key(Key::Semicolon) + Key::Alt, "\033;");
            KEY(Key(Key::Semicolon) + Key::Alt + Key::Shift, "\033:");
            KEY(Key(Key::Quote) + Key::Alt, "\033'");
            KEY(Key(Key::Quote) + Key::Alt + Key::Shift, "\033\"");
            KEY(Key(Key::Comma) + Key::Alt, "\033,");
            KEY(Key(Key::Comma) + Key::Alt + Key::Shift, "\033<");
            KEY(Key(Key::Dot) + Key::Alt, "\033.");
            KEY(Key(Key::Dot) + Key::Alt + Key::Shift, "\033>");
            KEY(Key(Key::Slash) + Key::Alt, "\033/");
            KEY(Key(Key::Slash) + Key::Alt + Key::Shift, "\033?");
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

            KEY(Key(Key::SquareOpen) + Key::Ctrl, "\033");
            KEY(Key(Key::Backslash) + Key::Ctrl, "\034");
            KEY(Key(Key::SquareClose) + Key::Ctrl, "\035");

            return km;
            #undef KEY
            #undef VT_MODIFIERS
        }

    } // anonymous namespace

    std::unordered_map<ui::Key, std::string> VT100::KeyMap_(InitializeVT100KeyMap());


    VT100::VT100(int x, int y, int width, int height, PTY * pty, size_t ptyBufferSize):
        Terminal(x, y, width, height, pty, ptyBufferSize),
        mouseMode_(MouseMode::Off),
        mouseEncoding_(MouseEncoding::Default),
        mouseLastButton_(0),
        mouseButtonsDown_(0),
        cursorMode_(CursorMode::Normal) {
    }

    void VT100::updateSize(int cols, int rows) {
        // TODO resize alternate buffer as well
        Terminal::updateSize(cols, rows);
    }

    void VT100::mouseDown(int col, int row, ui::MouseButton button, ui::Key modifiers) {
        ASSERT(mouseButtonsDown_ <= 3);
        ++mouseButtonsDown_;
		if (mouseMode_ != MouseMode::Off) {
            mouseLastButton_ = encodeMouseButton(button, modifiers);
            sendMouseEvent(mouseLastButton_, col, row, 'M');
            LOG(SEQ) << "Button " << button << " down at " << col << ";" << row;
        }
        Terminal::mouseUp(col, row, button, modifiers);
    }

    void VT100::mouseUp(int col, int row, ui::MouseButton button, ui::Key modifiers) {
        ASSERT(mouseButtonsDown_ > 0);
        --mouseButtonsDown_;
		if (mouseMode_ == MouseMode::Off) {
            mouseLastButton_ = encodeMouseButton(button, modifiers);
            sendMouseEvent(mouseLastButton_, col, row, 'm');
            LOG(SEQ) << "Button " << button << " up at " << col << ";" << row;
        }
        Terminal::mouseUp(col, row, button, modifiers);
    }

    void VT100::mouseWheel(int col, int row, int by, ui::Key modifiers) {
		if (mouseMode_ != MouseMode::Off) {
    		// mouse wheel adds 64 to the value
            mouseLastButton_ = encodeMouseButton((by > 0) ? ui::MouseButton::Left : ui::MouseButton::Right, modifiers) + 64;
            sendMouseEvent(mouseLastButton_, col, row, 'M');
            LOG(SEQ) << "Wheel offset " << by << " at " << col << ";" << row;
        }
        Terminal::mouseWheel(col, row, by, modifiers);
    }

    void VT100::mouseMove(int col, int row, ui::Key modifiers) {
        if (mouseMode_ != MouseMode::Off && (mouseMode_ != MouseMode::ButtonEvent || mouseButtonsDown_ != 0)) {
            // mouse move adds 32 to the last known button press
            sendMouseEvent(mouseLastButton_ + 32, col, row, 'M');
            LOG(SEQ) << "Mouse moved to " << col << ";" << row;
        }
        Terminal::mouseMove(col, row, modifiers);
    }

    void VT100::keyChar(helpers::Char c) {
		ASSERT(c.codepoint() >= 32);
        send(c.toCharPtr(), c.size());
        Terminal::keyChar(c);
    }

    void VT100::keyDown(ui::Key key) {
		std::string const* seq = GetSequenceForKey(key);
		if (seq != nullptr) {
			switch (key.code()) {
			case ui::Key::Up:
			case ui::Key::Down:
			case ui::Key::Left:
			case ui::Key::Right:
			case ui::Key::Home:
			case ui::Key::End:
				if (key.modifiers() == 0 && cursorMode_ == CursorMode::Application) {
					std::string sa(*seq);
					sa[1] = 'O';
					send(sa.c_str(), sa.size());
					return;
				}
				break;
			default:
				break;
			}
			send(seq->c_str(), seq->size());
		}
        Terminal::keyDown(key);
    }

    void VT100::keyUp(ui::Key key) {
        Terminal::keyUp(key);
    }

    size_t VT100::processInput(char * buffer, size_t bufferSize) {
        Buffer::Ptr guard(this->buffer()); // non-priority lock the buffer


        return bufferSize;

    }








	unsigned VT100::encodeMouseButton(ui::MouseButton btn, ui::Key modifiers) {
		unsigned result =
			((modifiers | ui::Key::Shift) ? 4 : 0) +
			((modifiers | ui::Key::Alt) ? 8 : 0) +
			((modifiers | ui::Key::Ctrl) ? 16 : 0);
		switch (btn) {
			case ui::MouseButton::Left:
				return result;
			case ui::MouseButton::Right:
				return result + 1;
			case ui::MouseButton::Wheel:
				return result + 2;
			default:
				UNREACHABLE;
		}
	}

	void VT100::sendMouseEvent(unsigned button, int col, int row, char end) {
		// first increment col & row since terminal starts from 1
		++col;
		++row;
		switch (mouseEncoding_) {
			case MouseEncoding::Default: {
				// if the event is release, button number is 3
				if (end == 'm')
					button |= 3;
				// increment all values so that we start at 32
				button += 32;
				col += 32;
				row += 32;
				// if the col & row are too high, ignore the event
				if (col > 255 || row > 255)
					return;
				// otherwise output the sequence
				char buffer[6];
				buffer[0] = '\033';
				buffer[1] = '[';
				buffer[2] = 'M';
				buffer[3] = button & 0xff;
				buffer[4] = col & 0xff;
				buffer[5] = row & 0xff;
				send(buffer, 6);
				break;
			}
			case MouseEncoding::UTF8: {
				LOG(SEQ_WONT_SUPPORT) << "utf8 mouse encoding";
				break;
			}
			case MouseEncoding::SGR: {
				std::string buffer = STR("\033[<" << button << ';' << col << ';' << row << end);
				send(buffer.c_str(), buffer.size());
				break;
			}
		}
	}



} // namespace vterm
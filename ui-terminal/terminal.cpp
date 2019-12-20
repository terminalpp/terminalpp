
#include "helpers/log.h"

#include "ui/key.h"

#include "terminal.h"

namespace ui {

    namespace {

        std::unordered_map<Key, std::string> InitializeVT100KeyMap() {
            #define KEY(K, ...) ASSERT(km.find(K) == km.end()) << "Key " << K << " altrady defined"; km.insert(std::make_pair(K, STR(__VA_ARGS__)))
            #define VT_MODIFIERS(K, SEQ1, SEQ2) KEY(Key(K) + Key::Shift, SEQ1 << 2 << SEQ2); \
                                                KEY(Key(K) + Key::Alt, SEQ1 << 3 << SEQ2); \
                                                KEY(Key(K) + Key::Shift + Key::Alt, SEQ1 << 4 << SEQ2); \
                                                KEY(Key(K) + Key::Ctrl, SEQ1 << 5 << SEQ2); \
                                                KEY(Key(K) + Key::Ctrl + Key::Shift, SEQ1 << 6 << SEQ2); \
                                                KEY(Key(K) + Key::Ctrl + Key::Alt, SEQ1 << 7 << SEQ2); \
                                                KEY(Key(K) + Key::Ctrl + Key::Alt + Key::Shift, SEQ1 << 8 << SEQ2);

            using namespace ui;

            std::unordered_map<Key, std::string> km;
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

    // Terminal::Palette
    Terminal::Palette Terminal::Palette::Colors16() {
		return Palette{
			Color::Black, // 0
			Color::DarkRed, // 1
			Color::DarkGreen, // 2
			Color::DarkYellow, // 3
			Color::DarkBlue, // 4
			Color::DarkMagenta, // 5
			Color::DarkCyan, // 6
			Color::Gray, // 7
			Color::DarkGray, // 8
			Color::Red, // 9
			Color::Green, // 10
			Color::Yellow, // 11
			Color::Blue, // 12
			Color::Magenta, // 13
			Color::Cyan, // 14
			Color::White // 15
		};
    }

    Terminal::Palette Terminal::Palette::XTerm256() {
        Palette result(256);
        // first the basic 16 colors
		result[0] =	Color::Black;
		result[1] =	Color::DarkRed;
		result[2] =	Color::DarkGreen;
		result[3] =	Color::DarkYellow;
		result[4] =	Color::DarkBlue;
		result[5] =	Color::DarkMagenta;
		result[6] =	Color::DarkCyan;
		result[7] =	Color::Gray;
		result[8] =	Color::DarkGray;
		result[9] =	Color::Red;
		result[10] = Color::Green;
		result[11] = Color::Yellow;
		result[12] = Color::Blue;
		result[13] = Color::Magenta;
		result[14] = Color::Cyan;
		result[15] = Color::White;
		// now do the xterm color cube
		unsigned i = 16;
		for (unsigned r = 0; r < 256; r += 40) {
			for (unsigned g = 0; g < 256; g += 40) {
				for (unsigned b = 0; b < 256; b += 40) {
					result[i] = Color(
						static_cast<unsigned char>(r),
						static_cast<unsigned char>(g),
						static_cast<unsigned char>(b)
					);
					++i;
					if (b == 0)
						b = 55;
				}
				if (g == 0)
					g = 55;
			}
			if (r == 0)
				r = 55;
		}
		// and finally do the grayscale
		for (unsigned char x = 8; x <= 238; x += 10) {
			result[i] = Color(x, x, x);
			++i;
		}
        return result;
    }

    Terminal::Palette::Palette(std::initializer_list<Color> colors, size_t defaultFg, size_t defaultBg):
        size_(colors.size()),
        defaultFg_(defaultFg),
        defaultBg_(defaultBg),
        colors_(new Color[colors.size()]) {
        ASSERT(defaultFg < size_ && defaultBg < size_);
		unsigned i = 0;
		for (Color c : colors)
			colors_[i++] = c;
    }

    Terminal::Palette::Palette(Palette const & from):
        size_(from.size_),
        defaultFg_(from.defaultFg_),
        defaultBg_(from.defaultBg_),
        colors_(new Color[from.size_]) {
		memcpy(colors_, from.colors_, sizeof(Color) * size_);
    }

    Terminal::Palette::Palette(Palette && from):
        size_(from.size_),
        defaultFg_(from.defaultFg_),
        defaultBg_(from.defaultBg_),
        colors_(from.colors_) {
        from.colors_ = nullptr;
        from.size_ = 0;
    }



    // Terminal::CSISequence

    Terminal::CSISequence Terminal::CSISequence::Parse(char * & start, char const * end) {
        CSISequence result;
        char * x = start;
        // if we are at the end, return incomplete
        if (x == end) {
            result.firstByte_ = INCOMPLETE;
            return result;
        }
        // parse the first byte
        if (IsParameterByte(*x) && *x != ';' && !helpers::IsDecimalDigit(*x))
            result.firstByte_ = *x++;
        ASSERT(result.firstByte_ != INVALID);
        // parse arguments, if any
        while (x != end && IsParameterByte(*x)) {
            // semicolon separates arguments, in this case an empty argument, which is initialized to default value (0)
            if (*x == ';') {
                ++x;
                result.args_.push_back(std::make_pair(DEFAULT_ARG_VALUE, false));
            // otherwise if we see digit, parse the argument given
            } else if (helpers::IsDecimalDigit(*x)) {
                int arg = 0;
                do {
                    arg = arg * 10 + helpers::DecCharToNumber(*x++);
                } while (x != end && helpers::IsDecimalDigit(*x));
                result.args_.push_back(std::make_pair(arg, true));
                // if there is separator, parse it as well
                if (x != end && *x == ';')
                    ++x;
            // other than numeric values are not supported for now
            } else {
                ++x;
                result.firstByte_ = INVALID;
            }
        }
        // parse intermediate bytes, if there are any, the sequence is marked as invalid because these are not supported now
		while (x != end && IsIntermediateByte(*x)) {
			result.firstByte_ = INVALID;
			++x;
		}
		// parse final byte, first check we are not at the end
		if (x == end) {
            result.firstByte_ = INCOMPLETE;
            return result;
        }
		if (IsFinalByte(*x))
			result.finalByte_ = *x++;
		else
			result.firstByte_ = INVALID;
        // log the sequence if invalid
        if (! result.valid())
			LOG(SEQ_UNKNOWN) << "Unknown, possibly invalid CSI sequence: \x1b" << std::string(start - 1 , x - start + 1);
        start = x;
        return result;
    }

    // Terminal::OSCSequence

    Terminal::OSCSequence Terminal::OSCSequence::Parse(char * & start, char const * end) {
        OSCSequence result;
        char * x = start;
        if (x == end) {
            result.num_ = INCOMPLETE;
            return result;
        }
        // parse the number
        if (helpers::IsDecimalDigit(*x)) {
            int arg = 0;
            do {
                arg = arg * 10 + helpers::DecCharToNumber(*x++);
            } while (x != end && helpers::IsDecimalDigit(*x));
            // if there is no semicolon, keep the INVALID in the number, but continue parsing to BEL or ST
            if (x != end && *x == ';') {
                    ++x;
                    result.num_ = arg;
            } 
        }
        // parse the value, which is terminated by either BEL, or ST, which is ESC followed by backslash
        char * valueStart = x;
        while (true) {
            if (x == end) {
                result.num_ = INCOMPLETE;
                return result;
            }
            // BEL
            if (*x == helpers::Char::BEL)
                break;
            // ST
            if (*x == helpers::Char::ESC && x + 1 != end && x[1] == '\\') {
                ++x;
                break;
            }
            // next
            ++x;
        }
        result.value_ = std::string(valueStart, x - valueStart);
        ++x; // past the terminating character
        start = x;
        return result;
    }

    // TerminalPP

    helpers::Log Terminal::SEQ("VT100");
    helpers::Log Terminal::SEQ_UNKNOWN("VT100_UNKNOWN");
    helpers::Log Terminal::SEQ_ERROR("VT100_ERROR");
    helpers::Log Terminal::SEQ_WONT_SUPPORT("VT100_WONT_SUPPORT");

    std::unordered_map<Key, std::string> Terminal::KeyMap_(InitializeVT100KeyMap());


    Terminal::Terminal(int width, int height, Palette const * palette, PTY * pty, unsigned fps, size_t ptyBufferSize):
        state_{width, height, palette->defaultForeground(), palette->defaultBackground()},
        mouseMode_{MouseMode::Off},
        mouseEncoding_{MouseEncoding::Default},
        mouseLastButton_{0},
        mouseButtonsDown_{0},
        cursorMode_{CursorMode::Normal},
        keypadMode_{KeypadMode::Normal},
        bracketedPaste_{false},
        alternateBufferMode_{false},
        alternateBuffer_{width, height, Cell{palette->defaultForeground(), palette->defaultBackground(), ' '}},
        alternateState_{width, height, palette->defaultForeground(), palette->defaultBackground()},
        palette_{palette},
        boldIsBright_{false},
        buffer_{width, height, Cell{palette->defaultForeground(), palette->defaultBackground(), ' '}},
        pty_{pty},
        fps_{fps},
        repaint_{false},
        scrollable_{true},
        scrollBarActive_{false},
        historySizeLimit_{0} {
        pty_->resize(width, height);
        ptyReader_ = std::thread([this, ptyBufferSize](){
            try {
                std::unique_ptr<char> holder(new char[ptyBufferSize]);
                char * ptyBuffer = holder.get();
                char * writeStart = holder.get();
                while (true) {
                    size_t read = pty_->receive(writeStart, ptyBufferSize - (writeStart - ptyBuffer));
                    // if 0 bytes were read, terminate the thread
                    if (read == 0)
                        break;
                    // otherwise add any pending data from previous cycle
                    read += (writeStart - ptyBuffer);
                    // process the input
                    size_t processed = read;
                    try {
                        processed = processInput(ptyBuffer, read);
                        // trigger the input processed event
                        onInput(this, InputProcessedEvent{ptyBuffer, processed});
                    } catch (std::exception const & e) {
                        onInputError(this, InputErrorEvent{ptyBuffer, processed, e.what()});
                    }
                    // if not everything was processed, copy the unprocessed part at the beginning and set writeStart_ accordingly
                    if (processed != read) {
                        memcpy(ptyBuffer, ptyBuffer + processed, read - processed);
                        writeStart = ptyBuffer + read - processed;
                        // check against buffer overflow here
                        if (writeStart == ptyBuffer + ptyBufferSize) {
                            LOG() << "Buffer overflow. Discarding";
                            writeStart = ptyBuffer;
                        }
                    } else {
                        writeStart = ptyBuffer;
                    }
                }
            } catch (...) {
                LOG() << "Error in reader";
            }
        });
        ptyListener_ = std::thread([this](){
            helpers::ExitCode ec = pty_->waitFor();
            this->ptyTerminated(ec);
        });
        repainter_ = std::thread([this](){
            while (fps_ > 0) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1000 / fps_));
                if (repaint_) {
                    repaint_ = false;
                    // trigger immediate repaint by calling widget's repaint implementation
                    Widget::repaint();
                }
            }
        });
    }

    Terminal::~Terminal() {
        fps_ = 0;
        delete pty_;
        ptyReader_.join();
        ptyListener_.join();
        repainter_.join();
        while (! history_.empty())
            popHistoryLine();
    }

    Color Terminal::defaultForeground() const {
        return palette_->defaultForeground();
    }
    
    Color Terminal::defaultBackground() const {
        return palette_->defaultBackground();
    }

    void Terminal::updateSize(int width, int height) {
        {
            Buffer::Ptr b = buffer(true); // grab priority lock
            b->resize(width, height, Cell{defaultForeground(), defaultBackground(), ' '},alternateBufferMode_ ? nullptr : this);
            // resize the real buffer
            state_.resize(width, height);
            // resize the alternate buffer and state
            alternateBuffer_.resize(width, height, Cell{defaultForeground(), defaultBackground(), ' '}, alternateBufferMode_ ? this : nullptr);
            alternateState_.resize(width, height);
        }
        resizeHistory(width);
        pty_->resize(width, height);
        // resize the client canvas
        setClientArea(width, buffer_.rows() + static_cast<int>(history_.size())); 
        ScrollBox::updateSize(width, height);
        Widget::updateSize(width, height);
    }

    void Terminal::paint(Canvas & canvas) {
        // determine the client canvas - either the proper client canvas if scrolling is available, or the child canvas
        Canvas clientCanvas{getClientCanvas(canvas)};
        int terminalOffset = static_cast<int>(history_.size());
        // draw the terminal if it is visible
        if (! scrollable_ || scrollOffset().y + height() > terminalOffset ) {
            Buffer::Ptr buffer = this->buffer(/* priority */true);
            clientCanvas.copyBuffer(0, terminalOffset, *buffer);
            // draw the cursor
            clientCanvas.setCursor(buffer->cursor() + Point{0, terminalOffset});
        }
        // if the terminal is scrollable, and there is any history, the scrollbar must be drawn and the history (if there is any visible)
        if (history_.size() > 0) {
           ASSERT(clientCanvas.height() == terminalOffset + height());
           for (int i = scrollOffset().y; i < terminalOffset; ++i) {
               clientCanvas.fill(Rect::FromCorners(0, i, width(), i + 1), defaultBackground());
               Cell * row = history_[i].second;
               for (int col = 0, ce = history_[i].first; col < ce; ++col)
                   clientCanvas.set(Point{col, i}, row[col]);
           }
           if (scrollable_) {
                std::pair<int, int> slider{verticalScrollbar(canvas.height())};
                canvas.drawRightVerticalScrollBar(Point{canvas.width() - 1, 0}, slider.first, slider.second, scrollBarActive_ ? Color::Red.withAlpha(128) : Color::White.withAlpha(64), scrollBarActive_);           
           }

           //drawVerticalScrollbarOverlay(canvas, scrollBarActive_ ? Color::Red.withAlpha(128) : Color::White.withAlpha(64), scrollBarActive_);
        }
        // paint the selection, if any
        paintSelection(clientCanvas, Color{128,128,192,128});
        // and finally, if the terminal is not enabled, dim its window accordingly
        if (!enabled())
            canvas.fill(Rect::FromWH(width(), height()), Brush(Color::Black.withAlpha(128)));
    }


    void Terminal::mouseDown(int col, int row, MouseButton button, Key modifiers) {
        ASSERT(mouseButtonsDown_ <= 3);
        ++mouseButtonsDown_;
		if (mouseMode_ != MouseMode::Off) {
            mouseLastButton_ = encodeMouseButton(button, modifiers);
            sendMouseEvent(mouseLastButton_, col, row, 'M');
            LOG(SEQ) << "Button " << button << " down at " << col << ";" << row;
        }
        if (modifiers == 0) {
            if (button == MouseButton::Left) {
                startSelection(Point{col, row} + scrollOffset());
            } else if (button == MouseButton::Wheel) {
                requestSelectionContents(); 
            } else if (button == MouseButton::Right && ! selection().empty()) {
                setClipboardContents(getSelectionContents());
                clearSelection();
            }
        }
        Widget::mouseDown(col, row, button, modifiers);
    }

    void Terminal::mouseUp(int col, int row, MouseButton button, Key modifiers) {
        ASSERT(mouseButtonsDown_ > 0);
        --mouseButtonsDown_;
		if (mouseMode_ != MouseMode::Off) {
            mouseLastButton_ = encodeMouseButton(button, modifiers);
            sendMouseEvent(mouseLastButton_, col, row, 'm');
            LOG(SEQ) << "Button " << button << " up at " << col << ";" << row;
        }
        if (modifiers == 0) {
            if (button == MouseButton::Left) 
                endSelection();
        }
        Widget::mouseUp(col, row, button, modifiers);
    }

    void Terminal::mouseWheel(int col, int row, int by, Key modifiers) {
		if (mouseMode_ != MouseMode::Off) {
    		// mouse wheel adds 64 to the value
            mouseLastButton_ = encodeMouseButton((by > 0) ? MouseButton::Left : MouseButton::Right, modifiers) + 64;
            sendMouseEvent(mouseLastButton_, col, row, 'M');
            LOG(SEQ) << "Wheel offset " << by << " at " << col << ";" << row;
        }
        if (scrollable_ && ! history_.empty()) {
            scrollBy(Point{0,-by});
            repaint();
        } 
        Widget::mouseWheel(col, row, by, modifiers);
    }

    void Terminal::mouseMove(int col, int row, Key modifiers) {
        if (mouseMode_ != MouseMode::Off && (mouseMode_ != MouseMode::ButtonEvent || mouseButtonsDown_ != 0)) {
            // mouse move adds 32 to the last known button press
            sendMouseEvent(mouseLastButton_ + 32, col, row, 'M');
            LOG(SEQ) << "Mouse moved to " << col << ";" << row;
        }
        if (modifiers == 0) {
            if (updatingSelection()) {
                updateSelection(Point{col, row} + scrollOffset(), Point{clientWidth(), clientHeight()});
                calculateVerticalAutoScroll(col, row);
            }
            bool x = col == width() - 1;
            if (x != scrollBarActive_) {
                scrollBarActive_ = x;
                repaint();
            }
        }
        Widget::mouseMove(col, row, modifiers);
    }

    void Terminal::keyChar(helpers::Char c) {
		ASSERT(c.codepoint() >= 32);
        send(c.toCharPtr(), c.size());
        Widget::keyChar(c);
    }

    void Terminal::keyDown(Key key) {
        // TODO this is ugly, fix help, make nice, etc, etc, etc, etc, etc. 
        if (key == (Key::V + Key::Ctrl + Key::Shift)) {
            requestClipboardContents();
            return;
        }
		std::string const* seq = GetSequenceForKey(key);
		if (seq != nullptr) {
			switch (key.code()) {
			case Key::Up:
			case Key::Down:
			case Key::Left:
			case Key::Right:
			case Key::Home:
			case Key::End:
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
        scrollToPrompt();
        Widget::keyDown(key);
    }

    void Terminal::keyUp(Key key) {
        Widget::keyUp(key);
    }

    /** TODO Selection works for CJK and double width characters, is a bit messy for double height characters, but not worth the time now, should be fixed when the selection is restructured and more pasting options are added. 
     
        Selection will insert spaces in double width characters which should span multiple columns as this feature is not supported by the renderer and so it is emulated in the terminal by inserting the spaces.

        TODO perhaps change this to not space, but something else so that we can ignore it from the selection? Or ignore this corner case entirely.
     */
    std::string Terminal::getSelectionContents() const {
        Selection s = selection();
        std::stringstream result;
        {
            // TODO this is an ugly hack for now, should be changed when new locking is implemented
            Buffer::Ptr buf = const_cast<Terminal*>(this)->buffer(); // lock the buffer (and therefore the history as well)
            int bufferStart = scrollable_ ? static_cast<int>(history_.size()) : 0;
            for (int y = s.start().y, ye = s.end().y; y < ye; ++y) {
                // determine the start (inclusive) and end (exclusive) column for the selection on current row
                int cStart = (y == s.start().y) ? s.start().x : 0;
                int cEnd = (y == s.end().y - 1) ? s.end().x : buf->cols();
                // update the column end based on the size of the line (if reading from history)
                cEnd = std::min(cEnd, (y >= bufferStart) ? buf->cols() : history_[y].first);
                // get the array of cells that corresponds to the current line (be it from buffer or history)
                Cell const * rowCells = (y >= bufferStart) ? & buf->at(0, y - bufferStart) : history_[y].second;
                
                // now create a UTF-8 string for the current line
                std::stringstream line;
                for (int i = cStart; i < cEnd; ) {
                    line << helpers::Char::FromCodepoint(rowCells[i].codepoint());
                    i += rowCells[i].font().width();
                }
                // add new lines to all lines but last
                if (y != s.start().y)
                    result << std::endl;
                // trim the line if it ended with empty characters
                result << helpers::TrimRight(line.str());
            }
        }
        return result.str();
    }

    void Terminal::lineScrolledOut(Cell * line, int cols) {
        if (! scrollable_) 
            return;
        if (historySizeLimit_ > 0) {
            addHistoryLine(line, cols);
            if (history_.size() > historySizeLimit_) {
                popHistoryLine();
            } else {
                updateClientRect();
                // TODO only scroll if we were at the top
                setScrollOffset(scrollOffset() + Point{0, 1});
            }
        }

        if (onLineScrolledOut.attached()) {
            buffer_.unlock();
            onLineScrolledOut(this);
            buffer_.lock();
        }
    }

    void Terminal::popHistoryLine() {
        ASSERT(! history_.empty());
        delete [] history_.front().second;
        history_.pop_front();
    }

    void Terminal::addHistoryLine(Cell const * line, int cols) {
        int x = cols - 1;
        Color bg = defaultBackground();
        while (x > 0) {
            Cell const & c = line[x];
            if (c.codepoint() != ' ')
                break;
            if (c.bg() != bg)
                break;
            --x;
        }
        ++x;
        Cell * row = new Cell[x];
        memcpy(row, line, sizeof(Cell) * x);
        history_.push_back(std::make_pair(x, row));
    }

    void Terminal::resizeHistory(int newCols) {
        for (auto i = history_.begin(), e = history_.end(); i != e; ) {
            if (i->first > newCols) {
                int restSize = i->first - newCols;
                Cell * line = new Cell[newCols];
                Cell * rest = new Cell[restSize];
                memcpy(line, i->second, sizeof(Cell) * newCols);
                memcpy(rest, i->second + newCols, sizeof(Cell) * restSize);
                delete [] i->second;
                i->first = newCols;
                i->second = line;
                i = history_.insert(++i, std::make_pair(restSize, rest));
                e = history_.end(); // all iterators invalidated by insert above
            } else {
                ++i;
            }
        }
        while (history_.size() > historySizeLimit_)
            popHistoryLine();
    }

    void Terminal::paste(std::string const & contents) {
        if (bracketedPaste_) {
			send("\033[200~", 6);
			send(contents);
			send("\033[201~", 6);
        } else {
            send(contents);
        }
    }

    size_t Terminal::processInput(char * buffer, size_t bufferSize) {
        {
            Buffer::Ptr guard(this->buffer()); // non-priority lock the buffer
            char * bufferEnd = buffer + bufferSize;
            char * x = buffer;
            while (x != bufferEnd) {
                switch (*x) {
                    /* Parse the escape sequence */
                    case helpers::Char::ESC: {
                        if (! parseEscapeSequence(x, bufferEnd)) {
                            repaint();
                            return x - buffer;
                        }
                        break;
                    }
                    /* BEL triggers the notification */
                    case helpers::Char::BEL: {
                        ++x;
                        // temporarily unlock the buffer as we are triggering the event, which is not high performance code
                        buffer_.unlock();
                        try {
                            notify();
                        } catch (std::exception const & e) {
                            LOG(SEQ_ERROR) << e.what();
                        } catch (...) {
                            LOG(SEQ_ERROR) << "unknown error";
                        }
                        buffer_.lock();
                        break;
                    }
                    case helpers::Char::TAB: {
						++x;
						updateCursorPosition();
						if (buffer_.cursor().pos.x % 8 == 0)
							buffer_.cursor().pos.x += 8;
						else
							buffer_.cursor().pos.x += 8 - (buffer_.cursor().pos.x % 8);
						LOG(SEQ) << "Tab: cursor col is " << buffer_.cursor().pos.x;
						break;
                    }
					/* New line simply moves to next line.
					 */
					case helpers::Char::LF: {
						LOG(SEQ) << "LF";
                        // disable double width and height chars
                        state_.cell.setFont(state_.cell.font().setSize(1).setDoubleWidth(false));
                        state_.doubleHeightTopLine = false;
                        // mark last position
						markLastCharPosition();
                        // move to next char in the input
						++x;
						// determine if region should be scrolled
						if (++buffer_.cursor().pos.y == state_.scrollEnd) {
							deleteLines(1, state_.scrollStart, state_.scrollEnd, (Cell(state_.cell).setAttributes(Attributes{})));
							--buffer_.cursor().pos.y;
						}
						updateCursorPosition();
						setLastCharPosition();
						break;
                    }
					/* Carriage return sets cursor column to 0.
					 */
					case helpers::Char::CR: {
						LOG(SEQ) << "CR";
						markLastCharPosition();
						++x;
						buffer_.cursor().pos.x = 0;
						break;
                    }
					case helpers::Char::BACKSPACE: {
						LOG(SEQ) << "BACKSPACE";
						++x;
						if (buffer_.cursor().pos.x == 0) {
							if (buffer_.cursor().pos.y > 0)
								--buffer_.cursor().pos.y;
							buffer_.cursor().pos.x = buffer_.cols() - 1;
						} else {
							--buffer_.cursor().pos.x;
						}
						break;
					}
  					/* default variant is to print the character received to current cell.
					 */
                    default: {
						// make sure that the cursor is in visible part of the screen
						updateCursorPosition();
						// it could be we are dealing with unicode. If entire character was not read, stop the processing, what we have so far will be prepended to next data to be processed
						helpers::Char const * c8 = helpers::Char::At(x, bufferEnd);
						if (c8 == nullptr)
							return x - buffer;
						LOG(SEQ) << "codepoint " << std::hex << c8->codepoint() << " " << static_cast<char>(c8->codepoint() & 0xff);
                        // get the cell and update its contents
                        Cell& cell = buffer_.at(buffer_.cursor().pos.x, buffer_.cursor().pos.y);
                        cell = state_.cell;
                        cell.setCodepoint(c8->codepoint());
                        // store the last character position
                        setLastCharPosition();
                        // move to next column
                        ++buffer_.cursor().pos.x;
                        // if the character's column width is 2 and current font is not double width, update to double width font
                        int columnWidth = c8->columnWidth();
                        if (columnWidth == 2 && ! cell.font().doubleWidth()) {
                            columnWidth = 1;
                            cell.setFont(cell.font().setDoubleWidth());
                        }
                        // if the font's size is greater than 1, copy the character as required (if we are at the top row of double height characters, increase the size artificially)
                        int charWidth = state_.doubleHeightTopLine ? cell.font().width() * 2 : cell.font().width();

                        while (columnWidth > 0 && buffer_.cursor().pos.x < buffer_.cols()) {
                            for (int i = 1; (i < charWidth) && buffer_.cursor().pos.x < buffer_.cols(); ++i) {
                                Cell& cell2 = buffer_.at(buffer_.cursor().pos);
                                // copy current cell properties
                                cell2 = cell;
                                // make sure the cell's font is normal size and width and display a space
                                cell2.setCodepoint(' ').setFont(cell.font().setSize(1).setDoubleWidth(false));
                                ++buffer_.cursor().pos.x;
                            } 
                            if (--columnWidth > 0 && buffer_.cursor().pos.x < buffer_.cols()) {
                                Cell& cell2 = buffer_.at(buffer_.cursor().pos);
                                // copy current cell properties
                                cell2 = cell;
                                cell2.setCodepoint(' ');
                                ++buffer_.cursor().pos.x;
                            } 
                        }
                    }
                }
            }
        }
        repaint();
        return bufferSize;
    }

    bool Terminal::parseEscapeSequence(char * & buffer, char const * bufferEnd) {
        ASSERT(*buffer == helpers::Char::ESC);
        char *x = buffer;
        ++x;
        // if we have nothing after the escape character, it is incomplete sequence
        if (x == bufferEnd)
            return false;
        // otherwise, based on the immediate character after ESC, determine what kind of sequence it is
        switch (*x++) {
            /* CSI Sequence */
            case '[': {
                CSISequence seq{CSISequence::Parse(x, bufferEnd)};
                // if the sequence is not valid, it has been reported already and we should just exit
                if (!seq.valid())
                    break;
                // if the sequence is not complete, return false and do not advance the buffer
                if (!seq.complete())
                    return false;
                // otherwise parse the CSI sequence
                parseCSISequence(seq);
                break;
            }
			/* Operating system command. */
			case ']': {
                OSCSequence seq{OSCSequence::Parse(x, bufferEnd)};
                // if the sequence is not valid, it has been reported already and we should just exit
                if (!seq.valid())
                    break;
                // if the sequence is not complete, return false and do not advance the buffer
                if (!seq.complete())
                    return false;
                parseOSCSequence(seq);
				break;
            }
			/* Save Cursor. */
			case '7':
				state_.cursorStack.push_back(Point(buffer_.cursor().pos.x, buffer_.cursor().pos.y));
				LOG(SEQ) << "DECSC: Cursor position saved";
				break;
			/* Restore Cursor. */
			case '8':
				if (!state_.cursorStack.empty()) {
					Point p = state_.cursorStack.back();
					setCursor(p.x, p.y);
					state_.cursorStack.pop_back();
					LOG(SEQ) << "DECRC: Cursor position restored";
				} else {
					LOG(SEQ_UNKNOWN) << "No cursor position to restore (DECRC)";
				}
				break;
			/* Reverse line feed - move up 1 row, same column.
			 */
			case 'M':
				LOG(SEQ) << "RI: move cursor 1 line up";
				if (buffer_.cursor().pos.y == state_.scrollStart) {
					buffer_.insertLines(1, state_.scrollStart, state_.scrollEnd, state_.cell);
				} else {
					setCursor(buffer_.cursor().pos.x, buffer_.cursor().pos.y - 1);
				}
				break;
            /* Device Control String (DCS). 
             */
            case 'P':
                if (x == bufferEnd)
                    return false;
                if (*x == '+') {
                    ++x;
                    tpp::Sequence seq(tpp::Sequence::Parse(x, bufferEnd));
                    if (!seq.complete())
                        return false;
                    if (!seq.valid())
                        break;
                    parseTppSequence(std::move(seq));
                }
                break;
    		/* Character set specification - ignored, we just have to parse it. */
			case '(':
			case ')':
			case '*':
			case '+':
				// missing character set specification
				if (x == bufferEnd)
					return false;
				if (*x == 'B') { // US
					++x;
					break;
				}
				LOG(SEQ_WONT_SUPPORT) << "Unknown (possibly mismatched) character set final char " << *x;
				++x;
				break;
			/* ESC = -- Application keypad */
			case '=':
				LOG(SEQ) << "Application keypad mode enabled";
                keypadMode_ = KeypadMode::Application;
				break;
			/* ESC > -- Normal keypad */
			case '>':
				LOG(SEQ) << "Normal keypad mode enabled";
                keypadMode_ = KeypadMode::Normal;
				break;
            /* ESC # number -- font size changes */
            case '#':
                if (x == bufferEnd)
                    return false;
                parseFontSizeSpecifier(*x);
                ++x;
                break;
            default:
				LOG(SEQ_UNKNOWN) << "Unknown escape sequence \x1b" << *(x-1);
				break;
        }
        buffer = x;
        return true;
    }

    void Terminal::parseCSISequence(CSISequence & seq) {
        switch (seq.firstByte()) {
            // the "normal" CSI sequences
            case 0: 
                switch (seq.finalByte()) {
                    // CSI <n> @ -- insert blank characters (ICH)
                    case '@':
                        seq.setDefault(0, 1);
                        LOG(SEQ) << "ICH: deleteCharacter " << seq[0];
                        insertCharacters(seq[0]);
                        return;
                    // CSI <n> A -- moves cursor n rows up (CUU)
                    case 'A': {
                        seq.setDefault(0, 1);
                        if (seq.numArgs() != 1)
                            break;
                        unsigned r = buffer_.cursor().pos.y >= seq[0] ? buffer_.cursor().pos.y - seq[0] : 0;
                        LOG(SEQ) << "CUU: setCursor " << buffer_.cursor().pos.x << ", " << r;
                        setCursor(buffer_.cursor().pos.x, r);
                        return;
                    }
                    // CSI <n> B -- moves cursor n rows down (CUD)
                    case 'B':
                        seq.setDefault(0, 1);
                        if (seq.numArgs() != 1)
                            break;
                        LOG(SEQ) << "CUD: setCursor " << buffer_.cursor().pos.x << ", " << buffer_.cursor().pos.y + seq[0];
                        setCursor(buffer_.cursor().pos.x, buffer_.cursor().pos.y + seq[0]);
                        return;
                    // CSI <n> C -- moves cursor n columns forward (right) (CUF)
                    case 'C':
                        seq.setDefault(0, 1);
                        if (seq.numArgs() != 1)
                            break;
                        LOG(SEQ) << "CUF: setCursor " << buffer_.cursor().pos.x + seq[0] << ", " << buffer_.cursor().pos.y;
                        setCursor(buffer_.cursor().pos.x + seq[0], buffer_.cursor().pos.y);
                        return;
                    // CSI <n> D -- moves cursor n columns back (left) (CUB)
                    case 'D': {// cursor backward
                        seq.setDefault(0, 1);
                        if (seq.numArgs() != 1)
                            break;
                        unsigned c = buffer_.cursor().pos.x >= seq[0] ? buffer_.cursor().pos.x - seq[0] : 0;
                        LOG(SEQ) << "CUB: setCursor " << c << ", " << buffer_.cursor().pos.y;
                        setCursor(c, buffer_.cursor().pos.y);
                        return;
                    }
                    /* CSI <n> G -- set cursor character absolute (CHA)
                    */
                    case 'G':
                        seq.setDefault(0, 1);
                        LOG(SEQ) << "CHA: set column " << seq[0] - 1;
                        setCursor(seq[0] - 1, buffer_.cursor().pos.y);
                        return;
                    /* set cursor position (CUP) */
                    case 'H': // CUP
                    case 'f': // HVP
                        seq.setDefault(0, 1).setDefault(1, 1);
                        if (seq.numArgs() != 2)
                            break;
                        seq.conditionalReplace(0, 0, 1);
                        seq.conditionalReplace(1, 0, 1);
                        LOG(SEQ) << "CUP: setCursor " << seq[1] - 1 << ", " << seq[0] - 1;
                        setCursor(seq[1] - 1, seq[0] - 1);
                        return;
                    /* CSI <n> J -- erase display, depending on <n>:
                        0 = erase from the current position (inclusive) to the end of display
                        1 = erase from the beginning to the current position(inclusive)
                        2 = erase entire display
                    */
                    case 'J':
                        if (seq.numArgs() > 1)
                            break;
                        switch (seq[0]) {
                            case 0:
                                updateCursorPosition();
                                fillRect(Rect::FromCorners(buffer_.cursor().pos.x, buffer_.cursor().pos.y, buffer_.cols(), buffer_.cursor().pos.y + 1), state_.cell);
                                fillRect(Rect::FromCorners(0, buffer_.cursor().pos.y + 1, buffer_.cols(), buffer_.rows()), state_.cell);
                                return;
                            case 1:
                                updateCursorPosition();
                                fillRect(Rect::FromCorners(0, 0, buffer_.cols(), buffer_.cursor().pos.y), state_.cell);
                                fillRect(Rect::FromCorners(0, buffer_.cursor().pos.y, buffer_.cursor().pos.x + 1, buffer_.cursor().pos.y + 1), state_.cell);
                                return;
                            case 2:
                                fillRect(Rect::FromWH(buffer_.cols(), buffer_.rows()), state_.cell);
                                return;
                            default:
                                break;
                        }
                        break;
                    /* CSI <n> K -- erase in line, depending on <n>
                        0 = Erase to Right
                        1 = Erase to Left
                        2 = Erase entire line
                    */
                    case 'K':
                        if (seq.numArgs() > 1)
                            break;
                        switch (seq[0]) {
                            case 0:
                                updateCursorPosition();
                                fillRect(Rect::FromCorners(buffer_.cursor().pos.x, buffer_.cursor().pos.y, buffer_.cols(), buffer_.cursor().pos.y + 1), state_.cell);
                                return;
                            case 1:
                                updateCursorPosition();
                                fillRect(Rect::FromCorners(0, buffer_.cursor().pos.y, buffer_.cursor().pos.x + 1, buffer_.cursor().pos.y + 1), state_.cell);
                                return;
                            case 2:
                                updateCursorPosition();
                                fillRect(Rect::FromCorners(0, buffer_.cursor().pos.y, buffer_.cols(), buffer_.cursor().pos.y + 1), state_.cell);
                                return;
                            default:
                                break;
                        }
					break;
                    /* CSI <n> L -- Insert n lines. (IL)
                     */
                    case 'L':
                        seq.setDefault(0, 1);
                        LOG(SEQ) << "IL: scrollUp " << seq[0];
                        buffer_.insertLines(seq[0], buffer_.cursor().pos.y, state_.scrollEnd, state_.cell);
                        return;
                    /* CSI <n> M -- Remove n lines. (DL)
                     */
                    case 'M':
                        seq.setDefault(0, 1);
                        LOG(SEQ) << "DL: scrollDown " << seq[0];
                        deleteLines(seq[0], buffer_.cursor().pos.y, state_.scrollEnd, state_.cell);
                        return;
                    /* CSI <n> P -- Delete n charcters. (DCH) 
                     */
                    case 'P':
                        seq.setDefault(0, 1);
                        LOG(SEQ) << "DCH: deleteCharacter " << seq[0];
                        deleteCharacters(seq[0]);
                        return;
                    /* CSI <n> S -- Scroll up n lines
                     */
                    case 'S':
                        seq.setDefault(0, 1);
                        LOG(SEQ) << "SU: scrollUp " << seq[0];
                        deleteLines(seq[0], state_.scrollStart, state_.scrollEnd, state_.cell);
                        return;
                    /* CSI <n> T -- Scroll down n lines
                     */
                    case 'T':
                        seq.setDefault(0, 1);
                        LOG(SEQ) << "SD: scrollDown " << seq[0];
                        buffer_.insertLines(seq[0], buffer_.cursor().pos.y, state_.scrollEnd, state_.cell);
                        return;
                    /* CSI <n> X -- erase <n> characters from the current position
                     */
                    case 'X': {
                        seq.setDefault(0, 1);
                        if (seq.numArgs() != 1)
                            break;
                        updateCursorPosition();
                        // erase from first line
                        int n = static_cast<unsigned>(seq[0]);
                        int l = std::min(buffer_.cols() - buffer_.cursor().pos.x, n);
                        fillRect(Rect::FromCorners(buffer_.cursor().pos.x, buffer_.cursor().pos.y, buffer_.cursor().pos.x + l, buffer_.cursor().pos.y + 1), state_.cell);
                        n -= l;
                        // while there is enough stuff left to be larger than a line, erase entire line
                        l = buffer_.cursor().pos.y + 1;
                        while (n >= buffer_.cols() && l < buffer_.rows()) {
                            fillRect(Rect::FromCorners(0, l, buffer_.cols(), l + 1), state_.cell);
                            ++l;
                            n -= buffer_.cols();
                        }
                        // if there is still something to erase, erase from the beginning
                        if (n != 0 && l < buffer_.rows())
                            fillRect(Rect::FromCorners(0, l, n, l + 1), state_.cell);
                        return;
                    }
                    /* CSI <n> c - primary device attributes.
                     */
                    case 'c': {
                        if (seq[0] != 0)
                            break;
                        LOG(SEQ) << "Device Attributes - VT102 sent";
                        send("\033[?6c", 5); // send VT-102 for now, go for VT-220? 
                        return;
                    }
                    /* CSI <n> d -- Line position absolute (VPA)
                     */
                    case 'd': {
                        seq.setDefault(0, 1);
                        if (seq.numArgs() != 1)
                            break;
                        int r = seq[0];
                        if (r < 1)
                            r = 1;
                        else if (r > buffer_.rows())
                            r = buffer_.rows();
                        LOG(SEQ) << "VPA: setCursor " << buffer_.cursor().pos.x << ", " << r - 1;
                        setCursor(buffer_.cursor().pos.x, r - 1);
                        return;
                    }
                    /* CSI <n> h -- Reset mode enable
                      
                       Depending on the argument, certain things are turned on. None of the RM settings are currently supported.
                     */
                    case 'h':
                        break;
                    /* CSI <n> l -- Reset mode disable
                    
                       Depending on the argument, certain things are turned off. Turning the features on/off is not allowed, but if the client wishes to disable something that is disabled, it's happily ignored.
                     */
                    case 'l':
                        seq.setDefault(0, 0);
                        // enable replace mode (IRM) since this is the only mode we allow, do nothing
                        if (seq[0] == 4)
                            return;
                        break;
                    /* SGR
                     */
                    case 'm':
                        return parseSGR(seq);
                    /* CSI <n> ; <n> r -- Set scrolling region (default is the whole window) (DECSTBM)
                     */
                    case 'r':
                        seq.setDefault(0, 1); // inclusive
                        seq.setDefault(1, buffer_.cursor().pos.y); // inclusive
                        if (seq.numArgs() != 2)
                            break;
                        // This is not proper 
                        seq.conditionalReplace(0, 0, 1);
                        seq.conditionalReplace(1, 0, 1);
                        if (seq[0] > buffer_.rows())
                            break;
                        if (seq[1] > buffer_.rows())
                            break;
                        state_.scrollStart = std::min(seq[0] - 1, buffer_.rows() - 1); // inclusive
                        state_.scrollEnd = std::min(seq[1], buffer_.rows()); // exclusive 
                        setCursor(0, 0);
                        LOG(SEQ) << "Scroll region set to " << state_.scrollStart << " - " << state_.scrollEnd;
                        return;
                    /* CSI <n> : <n> : <n> t -- window manipulation (xterm)

                        We do nothing for these at the moment, just recognize the few possibly interesting ones.
                     */
                    case 't':
                        seq.setDefault(0, 0).setDefault(1, 0).setDefault(2, 0);
                        switch (seq[0]) {
                        case 22:
                            // 22;0;0 -- save xterm icon and window title on stack
                            if (seq[1] == 0 && seq[2] == 0)
                                return;
                            break;
                        case 23:
                            // 23;0;0 -- restore xterm icon and window title from stack
                            if (seq[1] == 0 && seq[2] == 0)
                                return;
                            break;
                        default:
                            break;
                        }
                        break;
                    default:
                        break;
                }
                break;
            // getters and setters
            case '?':
                switch (seq.finalByte()) {
                    case 'h':
                        return parseCSIGetterOrSetter(seq, true);
                    case 'l':
                        return parseCSIGetterOrSetter(seq, false);
                    case 's':
                    case 'r':
                        return parseCSISaveOrRestore(seq);
                    default:
                        break;
                }
                break;
            // other CSI sequences
            case '>':
                switch (seq.finalByte()) {
    				/* CSI > 0 c -- Send secondary device attributes.
                     */
                    case 'c':
                        if (seq[0] != 0)
                            break;
					LOG(SEQ) << "Secondary Device Attributes - VT100 sent";
					send("\033[>0;0;0c", 9); // we are VT100, no version third must always be zero (ROM cartridge)
					return;
				default:
					break;
                }
                break;
            default:
                break;
        }
		LOG(SEQ_UNKNOWN) << " Unknown CSI sequence " << seq;
    }

    void Terminal::parseCSIGetterOrSetter(CSISequence & seq, bool value) {
		for (size_t i = 0; i < seq.numArgs(); ++i) {
			int id = seq[i];
			switch (id) {
				/* application cursor mode on/off
				 */
				case 1:
					cursorMode_ = value ? CursorMode::Application : CursorMode::Normal;
					LOG(SEQ) << "application cursor mode: " << value;
					continue;
				/* Smooth scrolling -- ignored*/
				case 4:
					LOG(SEQ_WONT_SUPPORT) << "Smooth scrolling: " << value;
					continue;
				/* DECAWM - autowrap mode on/off */
				case 7:
					if (value) {
						LOG(SEQ) << "autowrap mode enable (by default)";
                    } else {
						LOG(SEQ_UNKNOWN) << "CSI?7l, DECAWM does not support being disabled";
                    }
					continue;
				// cursor blinking
				case 12:
					buffer_.cursor().activeBlink = value;
					buffer_.cursor().inactiveBlink = value;
					LOG(SEQ) << "cursor blinking: " << value;
					continue;
				// cursor show/hide
				case 25:
					buffer_.cursor().visible = value;
					LOG(SEQ) << "cursor visible: " << value;
					continue;
				/* Mouse tracking movement & buttons.

				https://stackoverflow.com/questions/5966903/how-to-get-mousemove-and-mouseclick-in-bash
				*/
				/* Enable normal mouse mode, i.e. report button press & release events only.
				 */
				case 1000:
                    mouseMode_ = value ? MouseMode::Normal : MouseMode::Off;
					LOG(SEQ) << "normal mouse tracking: " << value;
					continue;
				/* Mouse highlighting - will not support because it requires supporting application and may hang terminal if not used properly, which sounds rather dangerous.
				 */
				case 1001:
					LOG(SEQ_WONT_SUPPORT) << "hilite mouse mode";
					continue;
				/* Mouse button events (report mouse button press & release and mouse movement if any of the buttons is down.
				 */
				case 1002:
                    mouseMode_ = value ? MouseMode::ButtonEvent : MouseMode::Off;
					LOG(SEQ) << "button-event mouse tracking: " << value;
					continue;
				/* Report all mouse events (i.e. report mouse move even when buttons are not pressed).
				 */
				case 1003:
                    mouseMode_ = value ? MouseMode::All : MouseMode::Off;
					LOG(SEQ) << "all mouse tracking: " << value;
					continue;
				/* UTF8 encoded tracking.
				 */
				case 1005:
					//mouseEncoding_ = value ? MouseEncoding::UTF8 : MouseEncoding::Default;
					LOG(SEQ_WONT_SUPPORT) << "UTF8 mouse encoding: " << value;
					continue;
				/* SGR mouse encoding.
				 */
				case 1006: // 
					mouseEncoding_ = value ? MouseEncoding::SGR : MouseEncoding::Default;
					LOG(SEQ) << "UTF8 mouse encoding: " << value;
					continue;
				/* Enable or disable the alternate screen buffer.
				 */
				case 47:
				case 1049:
					if (value) {
						// flip to alternate buffer and clear it
						if (!alternateBufferMode_) {
							alternateBuffer_ = buffer_;
							std::swap(state_, alternateState_);
							invalidateLastCharPosition();
                            // disable terminal history for alternate mode
                            enableScrolling(false);
						}
                        state_.cell.setFg(palette_->defaultForeground()) 
                                   .setDecoration(palette_->defaultForeground()) 
                                   .setBg(palette_->defaultBackground())
                                   .setFont(Font{})
                                   .setAttributes(Attributes{});
						fillRect(Rect::FromWH(buffer_.cols(), buffer_.rows()), state_.cell);
						buffer_.cursor().pos = Point{0,0};
                        buffer_.cursor().visible = true;
						LOG(SEQ) << "Alternate screen on";
					} else {
						// go back from alternate buffer
						if (alternateBufferMode_) {
							buffer_ = alternateBuffer_;
							std::swap(state_, alternateState_);
                            // enable history for normal mode
	                        enableScrolling();
					    }
						LOG(SEQ) << "Alternate screen off";
					}
					alternateBufferMode_ = value;
					continue;
				/* Enable/disable bracketed paste mode. When enabled, if user pastes code in the window, the contents should be enclosed with ESC [200~ and ESC[201~ so that the client app can determine it is contents of the clipboard (things like vi might otherwise want to interpret it. 
				 */
				case 2004:
					bracketedPaste_ = value;
					continue;
				default:
					break;
			}
			LOG(SEQ_UNKNOWN) << "Invalid Get/Set command: " << seq;
		}
    }

    void Terminal::parseCSISaveOrRestore(CSISequence & seq) {
		for (size_t i = 0; i < seq.numArgs(); ++i)
			LOG(SEQ_WONT_SUPPORT) << "Private mode " << (seq.finalByte() == 's' ? "save" : "restore") << ", id " << seq[i];
    }

    void Terminal::parseSGR(CSISequence & seq) {
        seq.setDefault(0, 0);
		for (size_t i = 0; i < seq.numArgs(); ++i) {
			switch (seq[i]) {
				/* Resets all attributes. */
				case 0:
                    state_.cell.setFg(palette_->defaultForeground()) 
                               .setDecoration(palette_->defaultForeground()) 
                               .setBg(palette_->defaultBackground())
                               .setFont(Font{})
                               .setAttributes(Attributes{});
                    LOG(SEQ) << "font fg bg reset";
					break;
				/* Bold / bright foreground. */
				case 1:
					state_.cell.setFont(state_.cell.font().setBold());
					LOG(SEQ) << "bold set";
					break;
				/* faint font (light) - won't support for now, though in theory we easily can. */
				case 2:
					LOG(SEQ_WONT_SUPPORT) << "faint font";
					break;
				/* Italics */
				case 3:
					state_.cell.setFont(state_.cell.font().setItalics());
					LOG(SEQ) << "italics set";
					break;
				/* Underline */
				case 4:
                    state_.cell.setAttributes(state_.cell.attributes().setUnderline());
					LOG(SEQ) << "underline set";
					break;
				/* Blinking text */
				case 5:
                    state_.cell.setAttributes(state_.cell.attributes().setBlink());
					LOG(SEQ) << "blink set";
					break;
				/* Inverse and inverse off */
				case 7:
				case 27: {
                    Color fg = state_.cell.fg();
                    Color bg = state_.cell.bg();
                    state_.cell.setFg(bg).setDecoration(bg).setBg(fg);
					LOG(SEQ) << "toggle inverse mode";
					break;
                }
				/* Strikethrough */
				case 9:
                    state_.cell.setAttributes(state_.cell.attributes().setStrikethrough());
					LOG(SEQ) << "strikethrough";
					break;
				/* Bold off */
				case 21:
					state_.cell.setFont(state_.cell.font().setBold(false));
					LOG(SEQ) << "bold off";
					break;
				/* Normal - neither bold, nor faint. */
				case 22:
					state_.cell.setFont(state_.cell.font().setBold(false).setItalics(false));
					LOG(SEQ) << "normal font set";
					break;
				/* Italics off. */
				case 23:
					state_.cell.setFont(state_.cell.font().setItalics(false));
					LOG(SEQ) << "italics off";
					break;
				/* Disable underline. */
				case 24:
                    state_.cell.setAttributes(state_.cell.attributes().setUnderline(false));
					LOG(SEQ) << "undeline off";
					break;
				/* Disable blinking. */
				case 25:
                    state_.cell.setAttributes(state_.cell.attributes().setBlink(false));
					LOG(SEQ) << "blink off";
					break;
				/* Disable strikethrough. */
				case 29:
                    state_.cell.setAttributes(state_.cell.attributes().setStrikethrough(false));
					LOG(SEQ) << "Strikethrough off";
					break;
				/* 30 - 37 are dark foreground colors, handled in the default case. */
				/* 38 - extended foreground color */
				case 38: {
                    Color fg = parseSGRExtendedColor(seq, i);
                    state_.cell.setFg(fg).setDecoration(fg);    
					LOG(SEQ) << "fg set to " << fg;
					break;
                }
				/* Foreground default. */
				case 39:
                    state_.cell.setFg(palette_->defaultForeground())
                               .setDecoration(palette_->defaultForeground());
					LOG(SEQ) << "fg reset";
					break;
				/* 40 - 47 are dark background color, handled in the default case. */
				/* 48 - extended background color */
				case 48: {
                    Color bg = parseSGRExtendedColor(seq, i);
                    state_.cell.setBg(bg);    
					LOG(SEQ) << "bg set to " << bg;
					break;
                }
				/* Background default */
				case 49:
					state_.cell.setBg(palette_->defaultBackground());
					LOG(SEQ) << "bg reset";
					break;
				/* 90 - 97 are bright foreground colors, handled in the default case. */
				/* 100 - 107 are bright background colors, handled in the default case. */
				default:
					if (seq[i] >= 30 && seq[i] <= 37) {
                        int colorIndex = seq[i] - 30;
                        if (boldIsBright_ && state_.cell.font().bold())
                            colorIndex += 8;
						state_.cell.setFg(palette_->at(colorIndex))
                                   .setDecoration(palette_->at(colorIndex));
						LOG(SEQ) << "fg set to " << palette_->at(seq[i] - 30);
					} else if (seq[i] >= 40 && seq[i] <= 47) {
						state_.cell.setBg(palette_->at(seq[i] - 40));
						LOG(SEQ) << "bg set to " << palette_->at(seq[i] - 40);
					} else if (seq[i] >= 90 && seq[i] <= 97) {
						state_.cell.setFg(palette_->at(seq[i] - 82))
                                   .setDecoration(palette_->at(seq[i] - 82));
						LOG(SEQ) << "fg set to " << palette_->at(seq[i] - 82);
					} else if (seq[i] >= 100 && seq[i] <= 107) {
						state_.cell.setBg(palette_->at(seq[i] - 92));
						LOG(SEQ) << "bg set to " << palette_->at(seq[i] - 92);
					} else {
						LOG(SEQ_UNKNOWN) << "Invalid SGR code: " << seq;
					}
					break;
			}
		}
    }

    Color Terminal::parseSGRExtendedColor(CSISequence & seq, size_t & i) {
		++i;
		if (i < seq.numArgs()) {
			switch (seq[i++]) {
				/* index from 256 colors */
				case 5:
					if (i >= seq.numArgs()) // not enough args 
						break;
					if (seq[i] > 255) // invalid color spec
						break;
                    return palette_->at(seq[i]);
				/* true color rgb */
				case 2:
					i += 2;
					if (i >= seq.numArgs()) // not enough args
						break;
					if (seq[i - 2] > 255 || seq[i - 1] > 255 || seq[i] > 255) // invalid color spec
						break;
					return Color(seq[i - 2] & 0xff, seq[i - 1] & 0xff, seq[i] & 0xff);
				/* everything else is an error */
				default:
					break;
			}
		}
		LOG(SEQ_UNKNOWN) << "Invalid extended color: " << seq;
		return Color::White;
    }

    void Terminal::parseOSCSequence(OSCSequence & seq) {
        switch (seq.num()) {
            /* OSC 0 - change the terminal title.
             */
            case 0:
    			LOG(SEQ) << "Title change to " << seq.value();
                buffer_.unlock();
                try {
                    setTitle(seq.value());
                } catch (std::exception const & e) {
                    LOG(SEQ_ERROR) << e.what();
                } catch (...) {
                    LOG(SEQ_ERROR) << "unknown error";
                }
                buffer_.lock();
                break;
            /* OSC 52 - set clipboard to given value. 
             */
            case 52:
                LOG(SEQ) << "Clipboard set to " << seq.value();
                buffer_.unlock();
                try {
                    setClipboardContents(seq.value());
                } catch (std::exception const & e) {
                    LOG(SEQ_ERROR) << e.what();
                } catch (...) {
                    LOG(SEQ_ERROR) << "unknown error";
                }
                buffer_.lock();
                break;
            /* OSC 112 - reset cursor color. 
             */
            case 112:
                NOT_IMPLEMENTED;
            default:
        		LOG(SEQ_UNKNOWN) << "Invalid OSC sequence: " << seq;
        }
    }

    void Terminal::parseTppSequence(tpp::Sequence && seq) {
        try {
            switch (seq.id()) {
                /* Returns the terminal capabilities.

                For now only returns version 1. In the future this should be parametrized by various capabilities that may or may not be implemented. 
                */
                case tpp::Sequence::Kind::Capabilities:
                    LOG(SEQ) << "t++ terminal capabilities request";
                    send(STR("\033P+"<< tpp::Sequence::Kind::Capabilities << ";0\007"));
                    break;
                case tpp::Sequence::Kind::NewFile: {
                    LOG(SEQ) << "t++ new file request";
                    TppNewFileEvent event{
                        tpp::Sequence::NewFileRequest{std::move(seq)},
                        tpp::Sequence::NewFileResponse{}
                    };
                    buffer_.unlock();
                    try {
                        onTppNewFile(this, event);
                    } catch (std::exception const & e) {
                        LOG(SEQ_ERROR) << e.what();
                    } catch (...) {
                        LOG(SEQ_ERROR) << "unknown error";
                    }
                    buffer_.lock();
                    send(STR("\033P+" << tpp::Sequence::Kind::NewFile << ';' << event.response.fileId << helpers::Char::BEL));
                    break;
                }
                case tpp::Sequence::Kind::Data: {
                    LOG(SEQ) << "t++ send request";
                    tpp::Sequence::DataRequest req{std::move(seq)};
                    buffer_.unlock();
                    try {
                        onTppData(this, req);
                    } catch (std::exception const & e) {
                        LOG(SEQ_ERROR) << e.what();
                    } catch (...) {
                        LOG(SEQ_ERROR) << "unknown error";
                    }
                    buffer_.lock();
                    break;
                }
                case tpp::Sequence::Kind::TransferStatus: {
                    LOG(SEQ) << "t++ transfer status";
                    TppTransferStatusEvent event{
                        tpp::Sequence::TransferStatusRequest{std::move(seq)},
                        tpp::Sequence::TransferStatusResponse{}
                    };
                    buffer_.unlock();
                    try {
                        onTppTransferStatus(this, event);
                    } catch (std::exception const & e) {
                        LOG(SEQ_ERROR) << e.what();
                    } catch (...) {
                        LOG(SEQ_ERROR) << "unknown error";
                    }
                    buffer_.lock();
                    send(STR("\033P+" << tpp::Sequence::Kind::TransferStatus << ";" << event.response.fileId << ";" << event.response.transmittedBytes << helpers::Char::BEL));
                    break;
                }
                case tpp::Sequence::Kind::OpenFile: {
                    LOG(SEQ) << "t++ file open request";
                    tpp::Sequence::OpenFileRequest req{std::move(seq)};
                    send(STR("\033P+" << tpp::Sequence::Kind::Ack << helpers::Char::BEL));
                    buffer_.unlock();
                    try {
                        onTppOpenFile(this, req);
                    } catch (std::exception const & e) {
                        LOG(SEQ_ERROR) << e.what();
                    } catch (...) {
                        LOG(SEQ_ERROR) << "unknown error";
                    }
                    buffer_.lock();
                    break;
                }
                default:
                    LOG(SEQ_UNKNOWN) << "Invalid t++ sequence: " << seq;
            }
        } catch (tpp::SequenceError const & e) {
            LOG(SEQ_ERROR) << e;
        } 
    }

    void Terminal::parseFontSizeSpecifier(char kind) {
        switch (kind) {
            /* DECDHL - double height line, top half

               The top line is ignored, we simply set the top line mark so that when characters are placed they will occupy the correct width.
             */
            case '3':
                state_.doubleHeightTopLine = true;
                state_.cell.setFont(state_.cell.font().setSize(1).setDoubleWidth(false));
                break;
            /* DECDHL - double height line, bottom half

               The bottom half actually sets the font as it is solely responsible for drawing the line.
             */
            case '4':
                state_.doubleHeightTopLine = false;
                state_.cell.setFont(state_.cell.font().setSize(2).setDoubleWidth(false));
                break;
            /* DECSWL - single width line (default)
             */
            case '5':
                state_.doubleHeightTopLine = false;
                state_.cell.setFont(state_.cell.font().setDoubleWidth(false).setSize(1));
                break;
            /* DECDWL - double width line
             */
            case '6':
                state_.doubleHeightTopLine = false;
                state_.cell.setFont(state_.cell.font().setDoubleWidth().setSize(1));
                break;
            /* DECALN which fills terminal with chars so that screen on VT100 could be aligned. 
             */
            case '8':
                LOG(SEQ_WONT_SUPPORT) << "DEC Screen Alignment test";
                break;
            default:
                LOG(SEQ_UNKNOWN) << "Invalid DEC font size specifier ESC # " << kind;
        }
    }

	unsigned Terminal::encodeMouseButton(MouseButton btn, Key modifiers) {
		unsigned result =
			((modifiers | Key::Shift) ? 4 : 0) +
			((modifiers | Key::Alt) ? 8 : 0) +
			((modifiers | Key::Ctrl) ? 16 : 0);
		switch (btn) {
			case MouseButton::Left:
				return result;
			case MouseButton::Right:
				return result + 1;
			case MouseButton::Wheel:
				return result + 2;
			default:
				UNREACHABLE;
		}
	}

	void Terminal::sendMouseEvent(unsigned button, int col, int row, char end) {
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

    void Terminal::updateCursorPosition() {
        int c = buffer_.cols();
        while (buffer_.cursor().pos.x >= c) {
            buffer_.cursor().pos.x -= c;
            if (++buffer_.cursor().pos.y == state_.scrollEnd) {
                deleteLines(1, state_.scrollStart, state_.scrollEnd, Cell(state_.cell).setAttributes(Attributes{}));
                --buffer_.cursor().pos.y;
            }
        }
        ASSERT(buffer_.cursor().pos.x < buffer_.cols());
        // if cursor row is not valid, just set it to the last row 
        if (buffer_.cursor().pos.y >= buffer_.rows())
            buffer_.cursor().pos.y = buffer_.rows() - 1;
    }

    void Terminal::setCursor(int col, int row) {
		buffer_.cursor().pos.x = col;
		buffer_.cursor().pos.y = row;
		// invalidate the last character position
		invalidateLastCharPosition();
    }

    void Terminal::fillRect(Rect const& rect, Cell const & cell) {
		LOG(SEQ) << "fillRect " << rect;
		for (int row = rect.top(); row < rect.bottom(); ++row) {
			for (int col = rect.left(); col < rect.right(); ++col) {
				Cell& c = buffer_.at(col, row);
                c = cell;
			}
		}
    }

    void Terminal::deleteCharacters(unsigned num) {
		unsigned r = buffer_.cursor().pos.y;
		for (unsigned c = buffer_.cursor().pos.x, e = buffer_.cols() - num; c < e; ++c) {
			Cell& cell = buffer_.at(c, r);
			cell = buffer_.at(c + num, r);
		}
		for (unsigned c = buffer_.cols() - num, e = buffer_.cols(); c < e; ++c) {
			Cell& cell = buffer_.at(c, r);
            cell = state_.cell;
		}
    }

    void Terminal::insertCharacters(unsigned num) {
		unsigned r = buffer_.cursor().pos.y;
		// first copy the characters
		for (unsigned c = buffer_.cols() - 1, e = buffer_.cursor().pos.x + num; c >= e; --c) {
			Cell& cell = buffer_.at(c, r);
			cell = buffer_.at(c - num, r);
		}
		for (unsigned c = buffer_.cursor().pos.x, e = buffer_.cursor().pos.x + num; c < e; ++c) {
			Cell& cell = buffer_.at(c, r);
            cell = state_.cell;
		}
    }

    void Terminal::deleteLines(int lines, int top, int bottom, Cell const & fill) {
        // if lines are deleted from the top row and alternate buffer is not enabled, notify the terminal history that given number of lines is about to be scrolled out
        while (lines-- > 0) {
            if (top == 0)
                lineScrolledOut(&buffer_.at(0,0), buffer_.cols());
            buffer_.deleteLines(1, top, bottom, fill);
        }
        /*
        if (top == 0)
            lineScrolledOut(lines);
        // delete the lines
        buffer_.deleteLines(lines, top, bottom, fill);
        */
    }

    // Terminal::Buffer

    Terminal::Buffer::Buffer(int cols, int rows, Cell const & fill):
        cols_{cols},
        rows_{rows},
        cells_{new Cell*[rows]} {
        for (int i = 0; i < rows; ++i) {
            cells_[i] = new Cell[cols];
            fillRow(cells_[i], fill, cols);
        }
    }

    Terminal::Buffer::Buffer(Buffer const & from):
        cols_{from.cols_},
        rows_{from.rows_},
        cells_{new Cell*[from.rows_]},
        cursor_{from.cursor_} {
        for (int i = 0; i < rows_; ++i) {
            cells_[i] = new Cell[cols_];
            memcpy(cells_[i], from.cells_[i], sizeof(Cell) * cols_);
        }
    }

    Terminal::Buffer & Terminal::Buffer::operator = (Buffer const & other) {
        // self - check
        if (this == &other)
            return *this;
        if (cols_ != other.cols_ || rows_ != other.rows_) {
            for (int i = 0; i < rows_; ++i)
                delete [] cells_[i];
            delete[] cells_;
            cols_ = other.cols_;
            rows_ = other.rows_;
            cells_ = new Cell*[rows_];
            for (int i = 0; i < rows_; ++i) 
                cells_[i] = new Cell[cols_];
        }
        for (int i = 0; i < rows_; ++i) 
            memcpy(cells_[i], other.cells_[i], sizeof(Cell) * cols_);
        cursor_ = other.cursor_;
        return *this;
    }

    Terminal::Buffer::~Buffer() {
        for (int i = 0; i < rows_; ++i)
            delete [] cells_[i];
        delete [] cells_;
    }

    void Terminal::Buffer::resize(int cols, int rows, Cell const & fill, Terminal * terminal) {
        if (cols_ != cols || rows_ != rows) {
            resizeCells(cols, rows, fill, terminal);
            cols_ = cols;
            rows_ = rows;
        }
    }

    void Terminal::Buffer::insertLines(int lines, int top, int bottom, Cell const & fill) {
        ASSERT(bottom <= rows_);
        for (int i = 0; i < lines; ++i) {
            Cell* row = cells_[bottom - 1];
            memmove(cells_ + top + 1, cells_ + top, sizeof(Cell*) * (bottom - top - 1));
            cells_[top] = row;
            fillRow(row, fill, cols_);
        }
    }

    void Terminal::Buffer::deleteLines(int lines, int top, int bottom, Cell const & fill) {
        ASSERT(bottom <= rows_);
        ASSERT(lines <= bottom - top);
        for (int i = 0; i < lines; ++i) {
            Cell* row = cells_[top];
            memmove(cells_ + top, cells_ + top + 1, sizeof(Cell*) * (bottom - top - 1));
            cells_[bottom - 1] = row;
            fillRow(row, fill, cols_);
        }
    }

    std::string Terminal::Buffer::getLine(int line) {
        ASSERT(line >= 0 && line < rows_);
        Cell * row = cells_[line];
        std::stringstream result;
        for (int i = 0; i < cols_; ++i)
            result << helpers::Char::FromCodepoint(row[i].codepoint());
        return result.str();
    }

    void Terminal::Buffer::fillRow(Cell* row, Cell const& fill, unsigned cols) {
        row[0] = fill;
        size_t i = 1;
        size_t next = 2;
        while (next < cols) {
            memcpy(row + i, row, sizeof(Cell) * i);
            i = next;
            next *= 2;
        }
        memcpy(row + i, row, sizeof(Cell) * (cols - i));
    }

	void Terminal::Buffer::resizeCells(int newCols, int newRows, Cell const & fill, Terminal * terminal) {
		// create the new cells 
		Cell** newCells = new Cell * [newRows];
		for (int i = 0; i < newRows; ++i) {
			newCells[i] = new Cell[newCols];
            fillRow(newCells[i], fill, newCols);
        }
		// now determine the row at which we should stop - this is done by going back from cursor's position until we hit end of line, that would be the last line we will use
		int stopRow = cursor_.pos.y - 1;
		while (stopRow >= 0) {
			Cell* row = cells_[stopRow];
			int i = 0;
			for (; i < cols_; ++i)
				if (Terminal::IsEndOfLine(row[i]))
					break;
			// we have found the line end
			if (i < cols_) {
				++stopRow; // stop after current row
				break;
			}
			// otherwise try the above line
			--stopRow;
		}
		// if not found, or out of bounds, we'll stop immediately
		if (stopRow < 0 )
			stopRow = 0;
		// now transfer the contents
		int oldCursorRow = cursor_.pos.y;
		cursor_.pos.x = 0;
		cursor_.pos.y = 0;
		for (int y = 0; y < stopRow; ++y) {
			for (int x = 0; x < cols_; ++x) {
				Cell& cell = cells_[y][x];
				newCells[cursor_.pos.y][cursor_.pos.x] = cell;
				// if the copied cell is end of line, or if we are at the end of new line, increase the cursor row
				if (Terminal::IsEndOfLine(cell) || ++cursor_.pos.x == newCols) {
					++cursor_.pos.y;
					cursor_.pos.x = 0;
				}
				// scroll the new lines if necessary
				if (cursor_.pos.y == newRows) {
					Cell* r = newCells[0];
                    // scroll the line out so that it will be added to the terminal's history
                    if (terminal != nullptr)
                        terminal->lineScrolledOut(r, newCols);
					memmove(newCells, newCells + 1, sizeof(Cell*) * (newRows - 1));
					newCells[newRows - 1] = r;
					fillRow(r, Cell(), newCols);
					--cursor_.pos.y;
				}
				// if it was new line, skip whatever was afterwards
				if (Terminal::IsEndOfLine(cell))
					break;
			}
		}
		// the contents was transferred, delete the old cells, replace them with the new ones
		for (int i = 0; i < rows_; ++i)
			delete[] cells_[i];
		delete[] cells_;
		cells_ = newCells;
		// because the first thing the app will do after resize is to adjust the cursor position if the current line span more than 1 terminal line, we must account for this and update cursor position
		cursor_.pos.y += (oldCursorRow - stopRow);
	}



} // namespace vterm
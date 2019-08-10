#if (defined ARCH_UNIX)
#include "x11_window.h"

namespace tpp {

	std::unordered_map<x11::Window, X11Window *> X11Window::Windows_;

    X11Window::X11Window(std::string const & title, int cols, int rows, unsigned baseCellHeightPx):
        RendererWindow<X11Window>(title, cols, rows, Font::GetOrCreate(ui::Font(), baseCellHeightPx)->cellWidthPx(),baseCellHeightPx) {

    }

    X11Window::~X11Window() {

    }

    void X11Window::show() {

    }

    void X11Window::updateFullscreen(bool value) {
        X11Application * app = X11Application::Instance();
		MotifHints hints;
		hints.flags = 2;
		if (value == true) {
            // get window size & position
            XWindowAttributes attrs;
            x11::Window childW; 
			XGetWindowAttributes(display_, window_, & attrs);
            XTranslateCoordinates(display_, window_, DefaultRootWindow(display_), 0, 0, & fullscreenRestore_.x, & fullscreenRestore_.y, &childW);
			hints.decorations = 0;
            fullscreenRestore_.width = attrs.width;
            fullscreenRestore_.height = attrs.height;
            fullscreenRestore_.x -= attrs.x;
            fullscreenRestore_.y -= attrs.y;
            // remove the decorations
            XChangeProperty(display_, window_, app->motifWmHints_, app->motifWmHints_, 32, PropModeReplace, (unsigned char*)& hints, 5);
            // update window size and position
            Screen * screen = XScreenOfDisplay(display_, XDefaultScreen(display_));
    		XMoveResizeWindow(display_, window_, 0, 0, XWidthOfScreen(screen), XHeightOfScreen(screen));
		} else {
			hints.decorations = 1;
            XChangeProperty(display_, window_, app->motifWmHints_, app->motifWmHints_, 32, PropModeReplace, (unsigned char*)& hints, 5);
    		XMoveResizeWindow(
                display_,
                window_,
                fullscreenRestore_.x,
                fullscreenRestore_.y,
                fullscreenRestore_.width,
                fullscreenRestore_.height
            );
		}
		XMapWindow(display_, window_);
        // actually update the value
        Window::updateFullscreen(value);
    }

    void X11Window::updateZoom(double value) {
		Font * f = Font::GetOrCreate(ui::Font(), static_cast<unsigned>(baseCellHeightPx_ * value));
		cellWidthPx_ = f->cellWidthPx();
		cellHeightPx_ = f->cellHeightPx();
		Window::updateZoom(value);
		Window::updateSizePx(widthPx_, heightPx_);
    }

    void X11Window::setIcon(unsigned long * icon) {
		XChangeProperty(
			display_,
			window_,
			X11Application::Instance()->netWmIcon_,
			XA_CARDINAL,
			32,
			PropModeReplace,
			reinterpret_cast<unsigned char*>(&icon[1]),
			icon[0]
		);
    }

    unsigned X11Window::GetStateModifiers(int state) {
		unsigned modifiers = 0;
		if (state & 1)
			modifiers += ui::Key::Shift;
		if (state & 4)
			modifiers += ui::Key::Ctrl;
		if (state & 8)
			modifiers += ui::Key::Alt;
		if (state & 64)
			modifiers += ui::Key::Win;
		return modifiers;
    }

    ui::Key X11Window::GetKey(KeySym k, unsigned modifiers, bool pressed) {
        if (k >= 'a' && k <= 'z') 
            return ui::Key(k - 32, modifiers);
        if (k >= 'A' && k <= 'Z')
            return ui::Key(k, modifiers);
        if (k >= '0' && k <= '9')
            return ui::Key(k, modifiers);
        // numpad
        if (k >= XK_0 && k <= XK_9)
            return ui::Key(ui::Key::Numpad0 + k - XK_0, modifiers);
        // fn keys
        if (k >= XK_F1 && k <= XK_F12)
            return ui::Key(ui::Key::F1 + k - XK_F1, modifiers);
        // others
        switch (k) {
            case XK_BackSpace:
                return ui::Key(ui::Key::Backspace, modifiers);
            case XK_Tab:
                return ui::Key(ui::Key::Tab, modifiers);
            case XK_Return:
                return ui::Key(ui::Key::Enter, modifiers);
            case XK_Caps_Lock:
                return ui::Key(ui::Key::CapsLock, modifiers);
            case XK_Escape:
                return ui::Key(ui::Key::Esc, modifiers);
            case XK_space:
                return ui::Key(ui::Key::Space, modifiers);
            case XK_Page_Up:
                return ui::Key(ui::Key::PageUp, modifiers);
            case XK_Page_Down:
                return ui::Key(ui::Key::PageDown, modifiers);
            case XK_End:
                return ui::Key(ui::Key::End, modifiers);
            case XK_Home:
                return ui::Key(ui::Key::Home, modifiers);
            case XK_Left:
                return ui::Key(ui::Key::Left, modifiers);
            case XK_Up:
                return ui::Key(ui::Key::Up, modifiers);
            case XK_Right:
                return ui::Key(ui::Key::Right, modifiers);
            case XK_Down:
                return ui::Key(ui::Key::Down, modifiers);
            case XK_Insert:
                return ui::Key(ui::Key::Insert, modifiers);
            case XK_Delete:
                return ui::Key(ui::Key::Delete, modifiers);
            case XK_Menu:
                return ui::Key(ui::Key::Menu, modifiers);
            case XK_KP_Multiply:
                return ui::Key(ui::Key::NumpadMul, modifiers);
            case XK_KP_Add:
                return ui::Key(ui::Key::NumpadAdd, modifiers);
            case XK_KP_Separator:
                return ui::Key(ui::Key::NumpadComma, modifiers);
            case XK_KP_Subtract:
                return ui::Key(ui::Key::NumpadSub, modifiers);
            case XK_KP_Decimal:
                return ui::Key(ui::Key::NumpadDot, modifiers);
            case XK_KP_Divide:
                return ui::Key(ui::Key::NumpadDiv, modifiers);
            case XK_Num_Lock:
                return ui::Key(ui::Key::NumLock, modifiers);
            case XK_Scroll_Lock:
                return ui::Key(ui::Key::ScrollLock, modifiers);
            case XK_semicolon:
                return ui::Key(ui::Key::Semicolon, modifiers);
            case XK_equal:
                return ui::Key(ui::Key::Equals, modifiers);
            case XK_comma:
                return ui::Key(ui::Key::Comma, modifiers);
            case XK_minus:
                return ui::Key(ui::Key::Minus, modifiers);
            case XK_period: // .
                return ui::Key(ui::Key::Dot, modifiers);
            case XK_slash:
                return ui::Key(ui::Key::Slash, modifiers);
            case XK_grave: // `
                return ui::Key(ui::Key::Tick, modifiers);
            case XK_bracketleft: // [
                return ui::Key(ui::Key::SquareOpen, modifiers);
            case XK_backslash:  
                return ui::Key(ui::Key::Backslash, modifiers);
            case XK_bracketright: // ]
                return ui::Key(ui::Key::SquareClose, modifiers);
            case XK_apostrophe: // '
                return ui::Key(ui::Key::Quote, modifiers);
			case XK_Shift_L:
			case XK_Shift_R:
				if (pressed)
					modifiers |= ui::Key::Shift;
				else
					modifiers &= ~ui::Key::Shift;
				return ui::Key(ui::Key::ShiftKey, modifiers);
			case XK_Control_L:
			case XK_Control_R:
				if (pressed)
					modifiers |= ui::Key::Ctrl;
				else
					modifiers &= ~ui::Key::Ctrl;
				return ui::Key(ui::Key::CtrlKey, modifiers);
			case XK_Alt_L:
			case XK_Alt_R:
				if (pressed)
					modifiers |= ui::Key::Alt;
				else
					modifiers &= ~ui::Key::Alt;
				return ui::Key(ui::Key::AltKey, modifiers);
			case XK_Meta_L:
			case XK_Meta_R:
				if (pressed)
					modifiers |= ui::Key::Win;
				else
					modifiers &= ~ui::Key::Win;
				return ui::Key(ui::Key::WinKey, modifiers);
			default:
                return ui::Key(ui::Key::Invalid, 0);
        }
    }

    void X11Window::EventHandler(XEvent & e) {

    }


} // namespace tpp

#endif
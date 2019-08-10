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

    }

    void X11Window::updateZoom(double value) {

    }

    void X11Window::setIcon(unsigned long * icon) {
        
    }

    unsigned X11Window::GetStateModifiers(int state) {

    }

    ui::Key X11Window::GetKey(KeySym k, unsigned modifiers, bool pressed) {

    }

    void X11Window::EventHandler(XEvent & e) {

    }


} // namespace tpp

#endif
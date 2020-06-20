#if (defined RENDERER_QT)
#include "qt_application.h"
#include "qt_window.h"

namespace tpp {

    QtWindow::QtWindow(std::string const & title, int cols, int rows):
        RendererWindow{cols, rows},
        font_{ nullptr },
        decorationBrush_{QColor{255, 255, 255, 255}},
        borderBrush_{QColor{255, 255, 255, 255}} {
        QWindowBase::resize(widthPx_, heightPx_);
        QWindowBase::setTitle(title.c_str());

        connect(this, &QtWindow::tppRequestUpdate, this, static_cast<void (QtWindow::*)()>(&QtWindow::update), Qt::ConnectionType::QueuedConnection);
        connect(this, &QtWindow::tppShowFullScreen, this, &QtWindow::showFullScreen, Qt::ConnectionType::QueuedConnection);
        connect(this, &QtWindow::tppShowNormal, this, &QtWindow::showNormal, Qt::ConnectionType::QueuedConnection);
        connect(this, &QtWindow::tppWindowClose, this, &QtWindow::close, Qt::ConnectionType::QueuedConnection);

        //setMouseTracking(true);
        RegisterWindowHandle(this, this);
        
        setTitle(title_);
        setIcon(icon_);
    }

    QtWindow::~QtWindow() {
        UnregisterWindowHandle(this);
    }

    void QtWindow::setTitle(std::string const & value) {
        RendererWindow::setTitle(value);
        QWindowBase::setTitle(QString{value.c_str()});
    }

    void QtWindow::setIcon(Window::Icon icon) {
        RendererWindow::setIcon(icon);
        QtApplication * app = QtApplication::Instance();
        switch (icon) {
            case Icon::Default:
                QWindowBase::setIcon(app->iconDefault_);
                app->setWindowIcon(app->iconDefault_);
                break;
            case Icon::Notification:
                QWindowBase::setIcon(app->iconNotification_);
                app->setWindowIcon(app->iconNotification_);
                break;
            default:
                UNREACHABLE;
        }
    }

    void QtWindow::show(bool value) {
        if (value)
            QWindowBase::show();
        else 
            QWindowBase::hide();
    }

    void QtWindow::requestClipboard(Widget * sender) {
        RendererWindow::requestClipboard(sender);
        QString text{QtApplication::Instance()->clipboard()->text(QClipboard::Mode::Clipboard)};
        rendererClipboardPaste(text.toStdString());
    }

    void QtWindow::requestSelection(Widget * sender) {
        RendererWindow::requestSelection(sender);
        QtApplication * app = QtApplication::Instance();
        if (app->clipboard()->supportsSelection()) {
            QString text{QtApplication::Instance()->clipboard()->text(QClipboard::Mode::Selection)};
            rendererSelectionPaste(text.toStdString());
        } else {
            if (app->selectionOwner_ != nullptr)
                rendererSelectionPaste(app->selection_);
        }
    }

    void QtWindow::rendererSetClipboard(std::string const & contents) {
        QtApplication::Instance()->setClipboard(contents);
    }

    void QtWindow::rendererRegisterSelection(std::string const & contents, Widget * owner) {
        QtApplication * app = QtApplication::Instance();
        QtWindow * oldOwner = app->selectionOwner_;
        // set the owner of the selection
        app->selectionOwner_ = this;
        // if there was a different owner before, clear its selection (since selection owner is already someone else, it will only clear the selection in the widget)
        if (oldOwner != nullptr && oldOwner != this)
            oldOwner->rendererClearSelection();
        // if selection is supported, update it, else store its contents in the application
        if (app->clipboard()->supportsSelection())
            app->clipboard()->setText(QString{contents.c_str()}, QClipboard::Mode::Selection);
        else
            app->selection_ = contents;
        // deal with the selection in own window
        RendererWindow::rendererRegisterSelection(contents, owner);
    }

    void QtWindow::rendererClearSelection() {
        QtApplication * app = QtApplication::Instance();
        if (app->selectionOwner_ == this) {
            app->selectionOwner_ = nullptr;
            if (app->clipboard()->supportsSelection())
                app->clipboard()->clear(QClipboard::Mode::Selection);
            else
                app->selection_.clear();
        }
        // deal with the selection clear in itself
        RendererWindow::rendererClearSelection();
    }

    void QtWindow::keyPressEvent(QKeyEvent* ev) {
        static_assert(sizeof(QChar) == sizeof(utf16_char));
        activeModifiers_ = Key{Key::Invalid, GetStateModifiers(ev->modifiers())};
        Key k{GetKey(ev->key(), activeModifiers_.modifiers(), true)};
        if (k != Key::Invalid)
            rendererKeyDown(k);
        // if ctrl, alt, or win is active, don't deal with keyChar
        if (k & ( Key::Ctrl + Key::Alt + Key::Win))
            return;
        // determine if there is a printable character to be sent
        QString str{ev->text()};
        if (!str.isEmpty()) {
            utf16_char const * data = pointer_cast<utf16_char const*>(str.data());
            Char c{Char::FromUTF16(data, data + str.size())};
            // raise the keyChar event
            // the delete character (ASCII 127) is also non printable, furthermore on macOS backspace incorrectly translates to it.
            if (c.codepoint() >= 32 && c.codepoint() != 127)
                rendererKeyChar(c);
        }
    }

    void QtWindow::keyReleaseEvent(QKeyEvent* ev) {
        activeModifiers_ = Key{Key::Invalid, GetStateModifiers(ev->modifiers())};
        Key k{GetKey(ev->key(), activeModifiers_.modifiers(), false)};
        if (k != Key::Invalid)
            rendererKeyUp(k);
    }

    void QtWindow::mousePressEvent(QMouseEvent * ev) {
        activeModifiers_ = Key{Key::Invalid, GetStateModifiers(ev->modifiers())};
        MouseButton btn;
        switch (ev->button()) {
            case Qt::MouseButton::LeftButton:
                btn = MouseButton::Left;
                break;
            case Qt::MouseButton::MiddleButton:
                btn = MouseButton::Wheel;
                break;
            case Qt::MouseButton::RightButton:
                btn = MouseButton::Right;
                break;
            default:
                // TODO should we call super? 
                return;
        }
        rendererMouseDown(Point{ev->x(), ev->y()}, btn, activeModifiers_);
    }

    void QtWindow::mouseReleaseEvent(QMouseEvent * ev) {
        activeModifiers_ = Key{Key::Invalid, GetStateModifiers(ev->modifiers())};
        MouseButton btn;
        switch (ev->button()) {
            case Qt::MouseButton::LeftButton:
                btn = MouseButton::Left;
                break;
            case Qt::MouseButton::MiddleButton:
                btn = MouseButton::Wheel;
                break;
            case Qt::MouseButton::RightButton:
                btn = MouseButton::Right;
                break;
            default:
                // TODO should we call super? 
                return;
        }
        rendererMouseUp(Point{ev->x(), ev->y()}, btn, activeModifiers_);
    }

    void QtWindow::mouseMoveEvent(QMouseEvent * ev) {
        activeModifiers_ = Key{Key::Invalid, GetStateModifiers(ev->modifiers())};
        rendererMouseMove(Point{ev->x(), ev->y()}, activeModifiers_);
    }

    void QtWindow::wheelEvent(QWheelEvent * ev) {
        activeModifiers_ = Key{Key::Invalid, GetStateModifiers(ev->modifiers())};
        // can't use pixelDelta as it is only high resolution scrolling information not available for regular mouse
        rendererMouseWheel(Point{ev->x(), ev->y()}, (ev->angleDelta().y() > 0) ? 1 : -1, activeModifiers_);
    }

    void QtWindow::focusInEvent(QFocusEvent * ev) {
        QWindowBase::focusInEvent(ev);
        rendererFocusIn();
    }

    void QtWindow::focusOutEvent(QFocusEvent * ev) {
        QWindowBase::focusOutEvent(ev);
        rendererFocusOut();
    }

    unsigned QtWindow::GetStateModifiers(Qt::KeyboardModifiers const & modifiers) {
        unsigned result = 0;
        if (modifiers & Qt::KeyboardModifier::ShiftModifier)
            result += ui::Key::Shift;
        if (modifiers & Qt::KeyboardModifier::ControlModifier)
            result += ui::Key::Ctrl;
        if (modifiers & Qt::KeyboardModifier::AltModifier)
            result += ui::Key::Alt;
        if (modifiers & Qt::KeyboardModifier::MetaModifier)
            result += ui::Key::Win;
        return result;
    }

    // TODO check if shift meta and friends must do anything with the pressed
    Key QtWindow::GetKey(int qtKey, unsigned modifiers, bool pressed) {
        MARK_AS_UNUSED(pressed);
        if (qtKey >= Qt::Key::Key_A && qtKey <= Qt::Key::Key_Z) 
            return Key{static_cast<unsigned>(qtKey), modifiers};
        if (qtKey >= Qt::Key::Key_0 && qtKey <= Qt::Key::Key_9) 
            return Key{static_cast<unsigned>(qtKey), modifiers};
        switch (qtKey) {
            case Qt::Key::Key_Backspace:
                return Key{Key::Backspace, modifiers};
            case Qt::Key::Key_Tab:
                return Key{Key::Tab, modifiers};
            case Qt::Key::Key_Enter:
            case Qt::Key::Key_Return:
                return Key{Key::Enter, modifiers};
            case Qt::Key::Key_Shift:
                return Key{Key::ShiftKey, modifiers};
            case Qt::Key::Key_Control:
                return Key{Key::CtrlKey, modifiers};
            case Qt::Key::Key_Alt:
            case Qt::Key::Key_AltGr:
                return Key{Key::AltKey, modifiers};
            case Qt::Key::Key_CapsLock:
                return Key{Key::CapsLock, modifiers};
            case Qt::Key::Key_Escape:
                return Key{Key::Esc, modifiers};
            case Qt::Key::Key_Space:
                return Key{Key::Space, modifiers};
            case Qt::Key::Key_PageUp:
                return Key{Key::PageUp, modifiers};
            case Qt::Key::Key_PageDown:
                return Key{Key::PageDown, modifiers};
            case Qt::Key::Key_End:
                return Key{Key::End, modifiers};
            case Qt::Key::Key_Home:
                return Key{Key::Home, modifiers};
            case Qt::Key::Key_Left:
                return Key{Key::Left, modifiers};
            case Qt::Key::Key_Up:
                return Key{Key::Up, modifiers};
            case Qt::Key::Key_Right:
                return Key{Key::Right, modifiers};
            case Qt::Key::Key_Down:
                return Key{Key::Down, modifiers};
            case Qt::Key::Key_Insert:
                return Key{Key::Insert, modifiers};
            case Qt::Key::Key_Delete:
                return Key{Key::Delete, modifiers};
            case Qt::Key::Key_Meta:
                return Key{Key::WinKey, modifiers};
            case Qt::Key::Key_Menu:
                return Key{Key::Menu, modifiers};
            // TODO deal with numeric keyboard
            case Qt::Key::Key_F1:
                return Key{Key::F1, modifiers};
            case Qt::Key::Key_F2:
                return Key{Key::F2, modifiers};
            case Qt::Key::Key_F3:
                return Key{Key::F3, modifiers};
            case Qt::Key::Key_F4:
                return Key{Key::F4, modifiers};
            case Qt::Key::Key_F5:
                return Key{Key::F5, modifiers};
            case Qt::Key::Key_F6:
                return Key{Key::F6, modifiers};
            case Qt::Key::Key_F7:
                return Key{Key::F7, modifiers};
            case Qt::Key::Key_F8:
                return Key{Key::F8, modifiers};
            case Qt::Key::Key_F9:
                return Key{Key::F9, modifiers};
            case Qt::Key::Key_F10:
                return Key{Key::F10, modifiers};
            case Qt::Key::Key_F11:
                return Key{Key::F11, modifiers};
            case Qt::Key::Key_F12:
                return Key{Key::F12, modifiers};
            case Qt::Key::Key_NumLock:
                return Key{Key::NumLock, modifiers};
            case Qt::Key::Key_ScrollLock:
                return Key{Key::ScrollLock, modifiers};
            case Qt::Key::Key_Semicolon:
                return Key{Key::Semicolon, modifiers};
            case Qt::Key::Key_Equal:
                return Key{Key::Equals, modifiers};
            case Qt::Key::Key_Comma:
                return Key{Key::Comma, modifiers};
            case Qt::Key::Key_Minus:
                return Key{Key::Minus, modifiers};
            case Qt::Key::Key_Period:
                return Key{Key::Dot, modifiers};
            case Qt::Key::Key_Slash:
                return Key{Key::Slash, modifiers};
            // TODO tick (tilde)
            case Qt::Key::Key_BraceLeft:
                return Key{Key::SquareOpen, modifiers};
            case Qt::Key::Key_Backslash:
                return Key{Key::Backslash, modifiers};
            case Qt::Key::Key_BraceRight:
                return Key{Key::SquareClose, modifiers};
            // TODO Quote
            default:
                return Key{Key::Invalid, modifiers};
        }
    }

} // namespace tpp

#endif

#if (defined RENDERER_QT)
#include "qt_application.h"
#include "qt_window.h"

namespace tpp {

    QtWindow::QtWindow(std::string const & title, int cols, int rows, EventQueue & eventQueue):
        RendererWindow{cols, rows, eventQueue},
        font_{ nullptr },
        decorationBrush_{QColor{255, 255, 255, 255}},
        borderBrush_{QColor{255, 255, 255, 255}} {
        QWidget::resize(sizePx_.width(), sizePx_.height());
        QWidget::setWindowTitle(title.c_str());
        setFocusPolicy(Qt::StrongFocus);

        connect(this, &QtWindow::tppRequestUpdate, this, static_cast<void (QtWindow::*)()>(&QtWindow::update), Qt::ConnectionType::QueuedConnection);
        connect(this, &QtWindow::tppShowFullScreen, this, &QtWindow::showFullScreen, Qt::ConnectionType::QueuedConnection);
        connect(this, &QtWindow::tppShowNormal, this, &QtWindow::showNormal, Qt::ConnectionType::QueuedConnection);

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
        QWidget::setWindowTitle(QString{value.c_str()});
    }

    void QtWindow::setIcon(Window::Icon icon) {
        RendererWindow::setIcon(icon);
        QtApplication * app = QtApplication::Instance();
        switch (icon) {
            case Icon::Default:
                QWidget::setWindowIcon(app->iconDefault_);
                app->setWindowIcon(app->iconDefault_);
                break;
            case Icon::Notification:
                QWidget::setWindowIcon(app->iconNotification_);
                app->setWindowIcon(app->iconNotification_);
                break;
            default:
                UNREACHABLE;
        }
    }

    void QtWindow::show(bool value) {
        if (value)
            QWidget::show();
        else 
            QWidget::hide();
    }

    void QtWindow::requestClipboard(Widget * sender) {
        RendererWindow::requestClipboard(sender);
        QString text{QtApplication::Instance()->clipboard()->text(QClipboard::Mode::Clipboard)};
        pasteClipboard(text.toStdString());
    }

    void QtWindow::requestSelection(Widget * sender) {
        RendererWindow::requestSelection(sender);
        QtApplication * app = QtApplication::Instance();
        if (app->clipboard()->supportsSelection()) {
            QString text{QtApplication::Instance()->clipboard()->text(QClipboard::Mode::Selection)};
            pasteSelection(text.toStdString());
        } else {
            if (app->selectionOwner_ != nullptr)
                pasteSelection(app->selection_);
        }
    }

    void QtWindow::setClipboard(std::string const & contents) {
        QtApplication::Instance()->setClipboard(contents);
    }

    void QtWindow::setSelection(std::string const & contents, Widget * owner) {
        QtApplication * app = QtApplication::Instance();
        QtWindow * oldOwner = app->selectionOwner_;
        // set the owner of the selection
        app->selectionOwner_ = this;
        // if there was a different owner before, clear its selection (since selection owner is already someone else, it will only clear the selection in the widget)
        if (oldOwner != nullptr && oldOwner != this)
            oldOwner->clearSelection(nullptr);
        // if selection is supported, update it, else store its contents in the application
        if (app->clipboard()->supportsSelection())
            app->clipboard()->setText(QString{contents.c_str()}, QClipboard::Mode::Selection);
        else
            app->selection_ = contents;
    }

    void QtWindow::clearSelection(Widget * sender) {
        QtApplication * app = QtApplication::Instance();
        if (app->selectionOwner_ == this) {
            app->selectionOwner_ = nullptr;
            if (app->clipboard()->supportsSelection())
                app->clipboard()->clear(QClipboard::Mode::Selection);
            else
                app->selection_.clear();
        }
        // deal with the selection clear in itself
        RendererWindow::clearSelection(sender);
    }

    void QtWindow::keyPressEvent(QKeyEvent* ev) {
        static_assert(sizeof(QChar) == sizeof(utf16_char));
        setModifiers(GetStateModifiers(ev->modifiers()));
        Key k{GetKey(ev->key(), modifiers(), true)};
        if (k != Key::Invalid)
            keyDown(k);
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
                keyChar(c);
        }
    }

    void QtWindow::keyReleaseEvent(QKeyEvent* ev) {
        setModifiers(GetStateModifiers(ev->modifiers()));
        Key k{GetKey(ev->key(), modifiers(), false)};
        if (k != Key::Invalid)
            keyUp(k);
    }

    void QtWindow::mousePressEvent(QMouseEvent * ev) {
        setModifiers(GetStateModifiers(ev->modifiers()));
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
        mouseDown(pixelsToCoords(Point{ev->x(), ev->y()}), btn);
    }

    void QtWindow::mouseReleaseEvent(QMouseEvent * ev) {
        setModifiers(GetStateModifiers(ev->modifiers()));
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
        mouseUp(pixelsToCoords(Point{ev->x(), ev->y()}), btn);
    }

    void QtWindow::mouseMoveEvent(QMouseEvent * ev) {
        setModifiers(GetStateModifiers(ev->modifiers()));
        mouseMove(pixelsToCoords(Point{ev->x(), ev->y()}));
    }

    void QtWindow::wheelEvent(QWheelEvent * ev) {
        setModifiers(GetStateModifiers(ev->modifiers()));
        // can't use pixelDelta as it is only high resolution scrolling information not available for regular mouse
        mouseWheel(pixelsToCoords(Point{ev->x(), ev->y()}), (ev->angleDelta().y() > 0) ? 1 : -1);
    }

    void QtWindow::focusInEvent(QFocusEvent * ev) {
        QWidget::focusInEvent(ev);
        focusIn();
    }

    void QtWindow::focusOutEvent(QFocusEvent * ev) {
        QWidget::focusOutEvent(ev);
        focusOut();
    }

    void QtWindow::enterEvent(QEvent * ev) {
        QWidget::enterEvent(ev);
        mouseIn();
    }

    void QtWindow::leaveEvent(QEvent * ev) {
        QWidget::leaveEvent(ev);
        mouseOut();
    }


    ui::Key QtWindow::GetStateModifiers(Qt::KeyboardModifiers const & modifiers) {
        ui::Key result = ui::Key::Invalid;
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
    ui::Key QtWindow::GetKey(int qtKey, ui::Key modifiers, bool pressed) {
        MARK_AS_UNUSED(pressed);
        if (qtKey >= Qt::Key::Key_A && qtKey <= Qt::Key::Key_Z) 
            return Key::FromCode(static_cast<unsigned>(qtKey)) + modifiers;
        if (qtKey >= Qt::Key::Key_0 && qtKey <= Qt::Key::Key_9) 
            return Key::FromCode(static_cast<unsigned>(qtKey)) + modifiers;
        switch (qtKey) {
            case Qt::Key::Key_Backspace:
                return Key::Backspace + modifiers;
            case Qt::Key::Key_Tab:
                return Key::Tab + modifiers;
            case Qt::Key::Key_Enter:
            case Qt::Key::Key_Return:
                return Key::Enter + modifiers;
            case Qt::Key::Key_Shift:
                return Key::ShiftKey + modifiers;
            case Qt::Key::Key_Control:
                return Key::CtrlKey + modifiers;
            case Qt::Key::Key_Alt:
            case Qt::Key::Key_AltGr:
                return Key::AltKey + modifiers;
            case Qt::Key::Key_CapsLock:
                return Key::CapsLock + modifiers;
            case Qt::Key::Key_Escape:
                return Key::Esc + modifiers;
            case Qt::Key::Key_Space:
                return Key::Space + modifiers;
            case Qt::Key::Key_PageUp:
                return Key::PageUp + modifiers;
            case Qt::Key::Key_PageDown:
                return Key::PageDown + modifiers;
            case Qt::Key::Key_End:
                return Key::End + modifiers;
            case Qt::Key::Key_Home:
                return Key::Home + modifiers;
            case Qt::Key::Key_Left:
                return Key::Left + modifiers;
            case Qt::Key::Key_Up:
                return Key::Up + modifiers;
            case Qt::Key::Key_Right:
                return Key::Right + modifiers;
            case Qt::Key::Key_Down:
                return Key::Down + modifiers;
            case Qt::Key::Key_Insert:
                return Key::Insert + modifiers;
            case Qt::Key::Key_Delete:
                return Key::Delete + modifiers;
            case Qt::Key::Key_Meta:
                return Key::WinKey + modifiers;
            case Qt::Key::Key_Menu:
                return Key::Menu + modifiers;
            // TODO deal with numeric keyboard
            case Qt::Key::Key_F1:
                return Key::F1 + modifiers;
            case Qt::Key::Key_F2:
                return Key::F2 + modifiers;
            case Qt::Key::Key_F3:
                return Key::F3 + modifiers;
            case Qt::Key::Key_F4:
                return Key::F4 + modifiers;
            case Qt::Key::Key_F5:
                return Key::F5 + modifiers;
            case Qt::Key::Key_F6:
                return Key::F6 + modifiers;
            case Qt::Key::Key_F7:
                return Key::F7 + modifiers;
            case Qt::Key::Key_F8:
                return Key::F8 + modifiers;
            case Qt::Key::Key_F9:
                return Key::F9 + modifiers;
            case Qt::Key::Key_F10:
                return Key::F10 + modifiers;
            case Qt::Key::Key_F11:
                return Key::F11 + modifiers;
            case Qt::Key::Key_F12:
                return Key::F12 + modifiers;
            case Qt::Key::Key_NumLock:
                return Key::NumLock + modifiers;
            case Qt::Key::Key_ScrollLock:
                return Key::ScrollLock + modifiers;
            case Qt::Key::Key_Semicolon:
                return Key::Semicolon + modifiers;
            case Qt::Key::Key_Equal:
                return Key::Equals + modifiers;
            case Qt::Key::Key_Comma:
                return Key::Comma + modifiers;
            case Qt::Key::Key_Minus:
                return Key::Minus + modifiers;
            case Qt::Key::Key_Period:
                return Key::Dot + modifiers;
            case Qt::Key::Key_Slash:
                return Key::Slash + modifiers;
            // TODO tick (tilde)
            case Qt::Key::Key_BraceLeft:
                return Key::SquareOpen + modifiers;
            case Qt::Key::Key_Backslash:
                return Key::Backslash + modifiers;
            case Qt::Key::Key_BraceRight:
                return Key::SquareClose + modifiers;
            // TODO Quote
            default:
                return Key::Invalid + modifiers;
        }
    }

} // namespace tpp

#endif

#if (defined RENDERER_QT)

#include "../directwrite/windows.h"

#include "qt_application.h"
#include "qt_window.h"

namespace tpp {

    QtApplication::QtApplication(int & argc, char ** argv):
        QApplication{argc, argv},
        selectionOwner_{nullptr},
        iconDefault_{":/icon_32x32.png"},
        iconNotification_{":/icon-notification_32x32.png"} {
#if (defined ARCH_WINDOWS)
        // on windows, the console must be attached and its window disabled so that later executions of WSL programs won't spawn new console window
        AttachConsole();
#endif

        connect(clipboard(), &QClipboard::selectionChanged, this, &QtApplication::selectionChanged);
        connect(this, &QtApplication::tppUserEvent, this, static_cast<void (QtApplication::*)()>(&QtApplication::userEvent), Qt::ConnectionType::QueuedConnection);

        iconDefault_.addFile(":/icon_16x16.png");
        iconDefault_.addFile(":/icon_48x48.png");
        iconDefault_.addFile(":/icon_64x64.png");
        iconDefault_.addFile(":/icon_128x128.png");
        iconDefault_.addFile(":/icon_256x256.png");
        iconNotification_.addFile(":/icon-notification_16x16.png");
        iconNotification_.addFile(":/icon-notification_48x48.png");
        iconNotification_.addFile(":/icon-notification_64_64.png");
        iconNotification_.addFile(":/icon-notification_128x128.png");
        iconNotification_.addFile(":/icon-notification_256x256.png");
        // assertions to verify that the qt resources were built properly
        ASSERT(QFile::exists(":/icon_32x32.png"));
        ASSERT(QFile::exists(":/icon-notification_32x32.png"));

		QtWindow::StartBlinkerThread();
    }

    Window * QtApplication::createWindow(std::string const & title, int cols, int rows) {
        return new QtWindow{title, cols, rows, eventQueue_};
    }

    void QtApplication::alert(std::string const & message) {
        QMessageBox msgBox{QMessageBox::Icon::Warning, "Error", message.c_str()};
        msgBox.exec();
    }

    bool QtApplication::query(std::string const & title, std::string const & message) {
        QMessageBox msgBox{QMessageBox::Icon::Question, title.c_str(), message.c_str()};
        msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
        return msgBox.exec() == QMessageBox::Yes;
    }

    void QtApplication::openLocalFile(std::string const & filename, bool edit) {
        MARK_AS_UNUSED(edit);
        QDesktopServices::openUrl(QUrl::fromLocalFile(filename.c_str()));
    }

    void QtApplication::openUrl(std::string const & url) {
        QDesktopServices::openUrl(QUrl::fromUserInput(url.c_str()));
    }

    void QtApplication::setClipboard(std::string const & contents) {
        QtApplication::Instance()->clipboard()->setText(QString{contents.c_str()}, QClipboard::Mode::Clipboard);
    }

    void QtApplication::userEvent() {
        eventQueue_.processEvent();        
    }

    void QtApplication::selectionChanged() {
        if (! clipboard()->ownsSelection() && selectionOwner_ != nullptr) {
            QtWindow * owner = selectionOwner_;
            selectionOwner_ = nullptr;
            ASSERT(selection_.empty());
            owner->clearSelection(nullptr);
        }
    }

} // namespace tpp

#endif

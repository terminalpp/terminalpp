#if (defined RENDERER_QT)

#include "../directwrite/windows.h"

#include "qt_application.h"
#include "qt_window.h"

namespace tpp {

    Window * QtApplication::createWindow(std::string const & title, int cols, int rows, unsigned cellHeightPx) {
        return new QtWindow{title, cols, rows, cellHeightPx};
    }

    void QtApplication::clearSelection(QtWindow * owner) {
        if (owner == selectionOwner_) {
            selectionOwner_ = nullptr;
            if (clipboard()->supportsSelection()) 
                clipboard()->clear(QClipboard::Mode::Selection);
            else
                selection_.clear();
        } else {
            LOG() << "Window renderer clear selection does not match stored selection owner";
        }
    }

    void QtApplication::setSelection(QString value, QtWindow * owner) {
        if (selectionOwner_ != owner && selectionOwner_ != nullptr) 
            selectionOwner_->selectionInvalidated();
        selectionOwner_ = owner;
        if (clipboard()->supportsSelection()) 
            clipboard()->setText(value, QClipboard::Mode::Selection);
        else
            selection_ = value.toStdString();
    }

    void QtApplication::getSelectionContents(QtWindow * sender) {
        if (clipboard()->supportsSelection()) {
            QString text{clipboard()->text(QClipboard::Mode::Selection)};
            if (! text.isEmpty())
                sender->paste(text.toStdString());
        } else {
            if (!selection_.empty())
                sender->paste(selection_);
        }
    }

    void QtApplication::getClipboardContents(QtWindow * sender) {
        QString text{clipboard()->text(QClipboard::Mode::Clipboard)};
        if (! text.isEmpty())
            sender->paste(text.toStdString());
    }

    void QtApplication::selectionChanged() {
        if (! clipboard()->ownsSelection() && selectionOwner_ != nullptr) {
            selectionOwner_->selectionInvalidated();
            selectionOwner_ = nullptr;
        }
    }

    QtApplication::QtApplication(int & argc, char ** argv):
        QApplication{argc, argv},
        selectionOwner_{nullptr},
        iconDefault_{":/resources/icon32.png"},
        iconNotification_{":/resources/icon-notification32.png"} {
#if (defined ARCH_WINDOWS)
        // on windows, the console must be attached and its window disabled so that later executions of WSL programs won't spawn new console window
        AttachConsole();
#endif

        connect(clipboard(), &QClipboard::selectionChanged, this, &QtApplication::selectionChanged);

        QtWindow::StartBlinkerThread();
        iconDefault_.addFile(":/resources/icon16.png");
        iconDefault_.addFile(":/resources/icon48.png");
        iconDefault_.addFile(":/resources/icon64.png");
        iconDefault_.addFile(":/resources/icon128.png");
        iconDefault_.addFile(":/resources/icon256.png");
        iconNotification_.addFile(":/resources/icon-notification16.png");
        iconNotification_.addFile(":/resources/icon-notification48.png");
        iconNotification_.addFile(":/resources/icon-notification64.png");
        iconNotification_.addFile(":/resources/icon-notification128.png");
        iconNotification_.addFile(":/resources/icon-notification256.png");
        // assertions to verify that the qt resources were built properly
        ASSERT(QFile::exists(":/resources/icon32.png"));
        ASSERT(QFile::exists(":/resources/icon-notification32.png"));
    }

    QtApplication::~QtApplication() {
    }

    void QtApplication::alert(std::string const & message) {
        QMessageBox msgBox{QMessageBox::Icon::Warning, "Error", message.c_str()};
        msgBox.exec();
    }

    void QtApplication::openLocalFile(std::string const & filename, bool edit) {
        MARK_AS_UNUSED(edit);
        QDesktopServices::openUrl(QUrl::fromLocalFile(filename.c_str()));
    }

} // namespace tpp

#endif

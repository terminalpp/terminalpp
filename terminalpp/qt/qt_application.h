#pragma once
#if (defined RENDERER_QT)

#include <QtWidgets>

#include "helpers/log.h"

#include "../application.h"

namespace tpp {

    class QtWindow;

    class QtApplication : public QApplication, public Application {
        Q_OBJECT
    public:

        static void Initialize(int & argc, char ** argv) {
            new QtApplication{argc, argv};
        }

        static QtApplication * Instance() {
            QtApplication * result = dynamic_cast<QtApplication*>(Application::Instance());
            ASSERT(result != nullptr);
            return result;
        }

        Window * createWindow(std::string const & title, int cols, int rows, unsigned cellHeightPx) override;

        void mainLoop() override {
            exec();
        }

        QIcon const & iconDefault() const {
            return iconDefault_;
        }

        QIcon const & iconNotification() const {
            return iconNotification_;
        }

    public slots:

        void clearSelection(QtWindow * owner);

        void setSelection(QString value, QtWindow * owner);

        /** Setting the clipboard is easy, just set the clipboard to the given string.
         */ 
        void setClipboard(QString value) {
            clipboard()->setText(value, QClipboard::Mode::Clipboard);
        }

        void getSelectionContents(QtWindow * sender);

        void getClipboardContents(QtWindow * sender);

    protected slots:

        void selectionChanged();
        
    protected:

        friend class QtWindow;

        QtApplication(int & argc, char ** argv);

        ~QtApplication() override;

        void alert(std::string const & message) override;

        void openLocalFile(std::string const & filename, bool edit) override;

        QtWindow * selectionOwner_;

        // the contents of the selection on platforms which do not support the selection buffer
        std::string selection_;

        QIcon iconDefault_;
        QIcon iconNotification_;

    }; // tpp::QtApplication

} // tpp

#endif
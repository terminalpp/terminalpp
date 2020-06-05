#pragma once
#if (defined RENDERER_QT)

#include <QtWidgets>

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
            return dynamic_cast<QtApplication*>(Application::Instance());
        }

        void alert(std::string const & message) override ;

        bool query(std::string const & title, std::string const & message) override;

        void openLocalFile(std::string const & filename, bool edit) override; 

        void openUrl(std::string const & url) override;

        void setClipboard(std::string const & contents) override;

        Window * createWindow(std::string const & title, int cols, int rows) override;

        void mainLoop() override {
            exec();
        }

        QIcon const & iconDefault() const {
            return iconDefault_;
        }

        QIcon const & iconNotification() const {
            return iconNotification_;
        }

    signals:
       void tppUserEvent();

    protected slots:

        void userEvent();

        void selectionChanged();

    protected:

    private:
        friend class QtFont;
        friend class QtWindow;

        QtApplication(int & argc, char ** argv);

        std::string selection_;
        QtWindow * selectionOwner_;

        QIcon iconDefault_;
        QIcon iconNotification_;
    }; // tpp::QtApplication

}

#endif
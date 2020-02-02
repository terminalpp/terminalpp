#if (defined RENDERER_QT)

#include "../directwrite/windows.h"

#include "qt_application.h"
#include "qt_window.h"

namespace tpp {

    Window * QtApplication::createWindow(std::string const & title, int cols, int rows, unsigned cellHeightPx) {
        return new QtWindow{title, cols, rows, cellHeightPx};
    }

    QtApplication::QtApplication(int & argc, char ** argv):
        QApplication{argc, argv} {
#if (defined ARCH_WINDOWS)
        // on windows, the console must be attached and its window disabled so that later executions of WSL programs won't spawn new console window
        AttachConsole();
#endif



        QtWindow::StartBlinkerThread();
    }
      

    QtApplication::~QtApplication() {
    }


    void QtApplication::alert(std::string const & message) {
        QMessageBox msgBox{QMessageBox::Icon::Warning, "Error", message.c_str()};
        msgBox.exec();
    }

    void QtApplication::openLocalFile(std::string const & filename, bool edit) {
        MARK_AS_UNUSED(filename);
        MARK_AS_UNUSED(edit);
        NOT_IMPLEMENTED;
    }

} // namespace tpp

#endif

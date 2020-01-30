#if (defined RENDERER_QT)

#include "qt_application.h"
#include "qt_window.h"

namespace tpp {

    Window * QtApplication::createWindow(std::string const & title, int cols, int rows, unsigned cellHeightPx) {
        return new QtWindow{title, cols, rows, cellHeightPx};
    }

    QtApplication::~QtApplication() {
    }

    void QtApplication::mainLoop() {
        NOT_IMPLEMENTED;
    }

    void QtApplication::alert(std::string const & message) {
        MARK_AS_UNUSED(message);
        NOT_IMPLEMENTED;
    }

    void QtApplication::openLocalFile(std::string const & filename, bool edit) {
        MARK_AS_UNUSED(filename);
        MARK_AS_UNUSED(edit);
        NOT_IMPLEMENTED;
    }

} // namespace tpp

#endif

#include "root_window.h"

#include "clipboard.h"

namespace ui {

    void Clipboard::requestClipboardPaste() {
        RootWindow * root = rootWindow();
        if (root)
            root->requestClipboardPaste(this);
    }

    void Clipboard::requestSelectionPaste() {
        RootWindow * root = rootWindow();
        if (root)
            root->requestSelectionPaste(this);
    }

    void Clipboard::setClipboard(std::string const & contents) {
        RootWindow * root = rootWindow();
        if (root)
            root->setClipboard(this, contents);
    }

    void Clipboard::setSelection(std::string const & contents) {
        if (selection_.empty()) {
            ASSERT(contents.empty());
            return;
        }
        ASSERT(! contents.empty());
        RootWindow * root = rootWindow();
        if (root)
            root->setSelection(this, contents);
    }

    void Clipboard::clearSelection() {
        if (selection_.empty())
            return;
        invalidateSelection();
        RootWindow * root = rootWindow();
        if (root)
            root->clearSelection(this);
    }


} // namespace ui

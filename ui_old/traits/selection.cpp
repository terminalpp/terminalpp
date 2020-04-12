#include "../root_window.h"

#include "selection.h"

namespace ui {

/*

    void SelectionOwner::clearSelection() {
        if (selection_.empty())
            return;
        RootWindow * root = rootWindow();
        if (root)
            root->clearSelection();
        else
            selectionInvalidated();
    }

    void SelectionOwner::setClipboard() {
        RootWindow * root = rootWindow();
        if (root)
            root->setClipboard(getSelectionContents());
    }

    void SelectionOwner::setClipboard(std::string const & value) {
        RootWindow * root = rootWindow();
        if (root)
            root->setClipboard(value);
    }

    void SelectionOwner::registerSelection() {
        if (selection_.empty()) {
            clearSelection();
        } else {
            RootWindow * root = rootWindow();
            if (root)
                root->registerSelection(this, getSelectionContents());
        }
    }
*/

} // namespace ui
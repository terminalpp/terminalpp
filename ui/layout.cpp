
#include "layout.h"
#include "container.h"

#include "layouts/maximize.h"

namespace ui {

    Layout * Layout::None = new MaximizeLayout();

    std::vector<Widget*> const & Layout::containerChildren(Container * container) {
        Container * c = dynamic_cast<Container*>(container); 
        ASSERT(c != nullptr);
        return c->children_;
    }

}
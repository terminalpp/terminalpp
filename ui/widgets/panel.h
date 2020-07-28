#pragma once

#include "../widget.h"

namespace ui {


    /** Basic 
     */
    class Panel : public virtual Widget {
    public:

        Panel() = default;

        Panel(Layout * layout) {
            setLayout(layout);
        }

        using Widget::setLayout;
        using Widget::attach;
        using Widget::attachBack;
        using Widget::detach;

    }; // ui::Panel

} // namespace ui
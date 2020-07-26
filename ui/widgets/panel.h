#pragma once

#include "../widget.h"

namespace ui {


    /** Basic 
     */
    class Panel : public Widget {
    public:

        using Widget::setLayout;
        using Widget::attach;
        using Widget::attachBack;
        using Widget::detach;

    }; // ui::Panel

} // namespace ui
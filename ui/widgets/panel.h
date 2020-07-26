#pragma once

#include "../styled_widget.h"

namespace ui {


    /** Basic 
     */
    class Panel : public StyledWidget {
    public:

        using StyledWidget::setLayout;
        using StyledWidget::attach;
        using StyledWidget::attachBack;
        using StyledWidget::detach;

    }; // ui::Panel

} // namespace ui
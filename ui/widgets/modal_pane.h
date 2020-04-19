#pragma once

#include "../container.h"

#include "../traits/widget_background.h"

namespace ui {

    class ModalPane : public Container, public WidgetBackground<ModalPane> {
    public:

        ModalPane():
            WidgetBackground{Color::Black.withAlpha(128)} {
        }

    protected:

        void paint(Canvas & canvas) {
            WidgetBackground::paint(canvas);
            Container::paint(canvas);
        }

    };


} // namespace ui
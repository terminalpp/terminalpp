#pragma once

#include "../container.h"
#include "../traits/box.h"

namespace ui {

    class Panel : public Container, public Box<Panel> {
    public:

    protected:

        void paint(Canvas & canvas) {
            Box::paint(canvas);
            // paint the children
            Container::paint(canvas);
        }

    }; // ui::Panel

} // namespace ui
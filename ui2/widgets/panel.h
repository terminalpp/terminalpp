#pragma once

#include "../container.h"

namespace ui2 {

    class Panel : public Container {
    public:

    protected:

        void paint(Canvas & canvas) {
            canvas.setBg(Color::Red);
            canvas.fillRect(canvas.rect());
            // paint the children
            Container::paint(canvas);
        }

    }; // ui::Panel

} // namespace ui
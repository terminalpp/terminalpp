#pragma once

#include "../container.h"

namespace ui2 {

    class Panel : public Container {
    public:

    protected:

        void paint(Canvas & canvas) {
            for (int x = 0; x < canvas.width(); ++x)
                for (int y = 0; y < canvas.height(); ++y) {

                }
        }

    }; // ui::Panel



} // namespace ui
#pragma once

#include "../container.h"
#include "../traits/box.h"

namespace ui {

    class Panel : public Container, public Box<Panel> {
    public:

    protected:

        bool delegatePaintToParent() override {
            return Box::delegatePaintToParent() || Container::delegatePaintToParent();
        }

        bool requireChildrenToDelegatePaint() override {
            return Box::requireChildrenToDelegatePaint() || Container::requireChildrenToDelegatePaint();
        }

        void paint(Canvas & canvas) override {
            Box::paint(canvas);
            // paint the children
            Container::paint(canvas);
        }

    }; // ui::Panel

} // namespace ui
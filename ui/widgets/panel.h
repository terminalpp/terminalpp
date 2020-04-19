#pragma once

#include "../container.h"
#include "../traits/widget_background.h"
#include "../traits/widget_border.h"

namespace ui {

    class Panel : public Container, public WidgetBackground<Panel>, public WidgetBorder<Panel> {
    public:

    protected:

        bool delegatePaintToParent() override {
            return WidgetBackground::delegatePaintToParent() || Container::delegatePaintToParent();
        }

        bool requireChildToDelegatePaint(Widget * child) override {
            return WidgetBorder::requireChildToDelegatePaint(child) || Container::requireChildToDelegatePaint(child);
        }

        void paint(Canvas & canvas) override {
            WidgetBackground::paint(canvas);
            WidgetBorder::paint(canvas);
            // paint the children
            Container::paint(canvas);
        }

    }; // ui::Panel

} // namespace ui
#pragma once

#include "../container.h"
#include "../traits/widget_background.h"
#include "../traits/widget_border.h"

namespace ui {

    class CustomPanel : public Container, public WidgetBackground<CustomPanel>, public WidgetBorder<CustomPanel> {
    public:

    protected:

        Widget * propagatePaintTarget(Widget * sender, Widget * target) override {
            return mergePaintTargets(target, {
                Container::propagatePaintTarget(sender, target),
                WidgetBackground::propagatePaintTarget(sender, target),
                WidgetBorder::propagatePaintTarget(sender, target)
            });
        }

        void paint(Canvas & canvas) override {
            WidgetBackground::paint(canvas);
            WidgetBorder::paint(canvas);
            // paint the children
            Container::paint(canvas);
        }

    }; // ui::CustomPanel

} // namespace ui
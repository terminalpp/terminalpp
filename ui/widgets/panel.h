#pragma once

#include "../container.h"
#include "../traits/widget_background.h"
#include "../traits/widget_border.h"

namespace ui {

    class CustomPanel : public Container, public WidgetBackground<CustomPanel>, public WidgetBorder<CustomPanel> {
    public:

    protected:

        bool isTransparent() override {
            return WidgetBackground::isTransparent() || Container::isTransparent();
        }

        bool requireRepaintParentFor(Widget * child) override {
            return WidgetBorder::requireRepaintParentFor(child) || Container::requireRepaintParentFor(child);
        }

        void paint(Canvas & canvas) override {
            WidgetBackground::paint(canvas);
            WidgetBorder::paint(canvas);
            // paint the children
            Container::paint(canvas);
        }

    }; // ui::Panel

} // namespace ui
#pragma once

#include "../container.h"

#include "../traits/widget_background.h"

namespace ui {

    /** Modal pane
     
        Starts invisible, then gets items 
     */
    class ModalPane : public Container, public WidgetBackground<ModalPane> {
    public:

        ModalPane():
            WidgetBackground{Color::Black.withAlpha(128)} {
            setVisible(false);
        }

        /** Displays the given widget modally. 
         */
        void show(Widget * widget) {
            add(widget);
            //widget->setRect(Rect::FromWH(width(), 10));
            setVisible(true);
        }

    protected:

        void paint(Canvas & canvas) {
            WidgetBackground::paint(canvas);
            Container::paint(canvas);
        }

    };


} // namespace ui
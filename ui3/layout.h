#pragma once

namespace ui3 {

    class Widget;

    // auto == autosize
    // layout == leave size to be set by layout completely
    // fixed == specified by the user, cannot be changed by layout
    // percentage == specified by percentage of size available by the layout
    class SizeHint {


    }; 

    /** Layout implementation. 
     */
    class Layout {
    public:
        /** Does the layout, can call move & resize only
         */
        void layout(Widget * widget);

        /** Calculates the overlay of immediate children of the given widget. 
         */
        void calculateOverlay(Widget * widget);

    };

} // namespace ui
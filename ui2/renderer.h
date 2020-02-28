#pragma once

#include "common.h"
#include "buffer.h"

namespace ui2 {

    class Widget;

    class Renderer {
    public:

        /** Requests repaint of the given widget. [thread-safe]
         
            The purpose of this method is to use whatever event queue (or other mechanism) the target rendering supports to schedule repaint of the specified widget in the main UI thread. 
         */
        virtual void repaint(Widget * widget) = 0;

    protected:

        friend class Canvas;

        /** Called when the selected rectangle of the backing buffer has been updated and needs rendered. 
         */
        virtual void render(Rect const & rect) = 0;

        /** Returns the renderer's backing buffer. 
         */
        Buffer & buffer() {
            UI_THREAD_CHECK;
            return buffer_;
        }

        /** Immediately repaints the given widget. 
         
            If the widget is overlaid with another widgets, the widget parent will be painted instead. When the painting is done, i.e. the buffer has been updated, the render method is called to actually render the update.  
         */
        void paintWidget(Widget * widget);

    private:

        Buffer buffer_;

        

    }; // ui::Renderer


} // namespace ui
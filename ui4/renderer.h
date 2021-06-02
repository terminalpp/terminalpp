#pragma once

#include "helpers/helpers.h"

#include "core/canvas.h"


#ifndef NDEBUG
#define IN_UI_THREAD (::ui::Renderer::UIThreadID() == std::this_thread::get_id())
#endif

namespace ui {

    /** Class responsible for rendering the UI widgets. 
     
     */
    class Renderer {
        friend class Canvas;
        friend class Widget;

    public:

        static constexpr size_t REPAINT_IMMEDIATE = 0;

        unsigned fps() const {
            return fps_;
        }

        void setFps(unsigned value);



    /** \name Renderer Interface
     
        The following methods must be implemented by specific renderers. They deal with mouse 
     */
    //@{
    protected:

        /** TODO Should be called periodically from the main loop. 
         */
        bool loop() {
        }

        virtual void idle() {

        }

        /** Called when a widget requests its update (repaint). 
         
            Based on the current FPS settings the renderer either repaints the widget immediately by calling own repaint() method, or waits for the next fps tick to repaint the closest parent of all scheduled widgets. 
         */
        void updateWidget(Widget * widget);

    private:

        /** Initiates immediate repaint of the given widget. 
         
            The method is called by either the updateWidget() method, or by the fps repaint trigger to repaint the widget. The default implementation simply calls Widget::Repaint for the widget. 

            The private virtual method exists only so that custom renderers can react to the repaint and actually render themselves properly. 
         */
        virtual void repaint(Widget * widget);

    //@}

    private:
        Buffer buffer_;

        std::atomic<unsigned> fps_{0}; 
        std::thread fpsThread_;

        Widget * repaintRoot_ = nullptr;

#ifndef NDEBUG
    public:
        /** Returns the UI thread id.
         */
        static std::thread::id UIThreadID() {
            static std::thread::id id{std::this_thread::get_id()};
            return id;
        }
#endif

    }; // ui::Renderer

} // namespace ui
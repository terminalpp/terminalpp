#pragma once

#include "helpers/helpers.h"

#include "core/canvas.h"


#ifndef NDEBUG
#define IN_UI_THREAD (::ui::Renderer::UIThreadID() == std::this_thread::get_id())
#endif

namespace ui {

    /** Class responsible for rendering the UI widgets. 
     
        Multiple renderers may exist and each renderer deals with its own rendering and event queue mechanism. 
     */
    class Renderer {
        friend class Canvas;
        friend class Widget;



    /** \name Renderer Interface
     
        The following methods must be implemented by specific renderers. They deal with mouse 
     */
    //@{
    protected:


    //@}

    private:
        Buffer buffer_;

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
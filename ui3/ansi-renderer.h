#pragma once

#include "helpers/ansi-terminal.h"

#include "renderer.h"

namespace ui3 {

    /** Renders the UI inside an ANSI escape sequences terminal. 
     */
    class AnsiRenderer : public Renderer {

    protected:

        void eventNotify() override {

        }

        void render(Buffer const & buffer, Rect const & rect) override {

        }




    }; // ui::AnsiRenderer

} // namespace ui3
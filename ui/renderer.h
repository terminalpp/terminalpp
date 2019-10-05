#pragma once

#include "helpers/helpers.h"

#include "widget.h"
#include "selection.h"
#include "root_window.h"

/** \page uirenderer ui - Renderer
  
    \brief Each UI root window can be attached to a Renderer, which is responsible for both rendering the window's contents when appropriate and for notifying the window of any I/O events (key presses, mouse, clipboard, etc.).

    \section rendererUserInput User Input

    \subsection rendererMouse Mouse

    Mouse movement, button press & release, wheel up & down are reported as well as when the mouse enters and leaves the root window's area. Mouse events are only returned when the mouse is captured by the root window, which happens either when the mouse is directly above the window (its cells), or when the mouse is locked - in which case the mouse can be anywhere on the screen, but the events will still be sent to the window. 

    Mouse is locked if a button was pressed while the mouse was captured by the window for as long as at least one button is still pressed. When mouse is locked, the enter & leave events are not received (whether mouse is within the window can be determined from the coordinates accompanying any other mouse events).

    TODO mouseEnter should happen before the first Move - check that it does.

    \subsection rendererKeyboard Keyboard 

 */

namespace ui {

    /** Base class for user interface renderers.
     */
    class Renderer {
    protected:
        friend class RootWindow;

        Renderer():
            rootWindow_(nullptr) {
        }

        virtual ~Renderer() {
            detach();
        }

        virtual int cols() const = 0;
        virtual int rows() const = 0;

        /** Request from the root window to close & detach the renderer. 
         
            In most cases this also means that the renderer will in due time dispose of the root window. 
         */
        virtual void requestClose() = 0;

        /** Paints the window, or its part. 
         */
        virtual void requestRender(ui::Rect const & rect) = 0;

        /** Sets the title of the rendered window. 
         */
        virtual void setTitle(std::string const & title) = 0;

        virtual void setIcon(RootWindow::Icon icon) = 0;

        virtual void requestClipboardContents() = 0;

        virtual void requestSelectionContents() = 0;

        /** Sets the contents of the clipboard to the given text. 
         */
        virtual void setClipboard(std::string const & contents) = 0;

        /** Sets the selection to given value. 
         */
        virtual void setSelection(std::string const & contents) = 0;

        /** Clears the selection. 
         */
        virtual void clearSelection() = 0;

        Canvas::Buffer::Ptr bufferToRender() {
            ASSERT(attached());
            return rootWindow_->buffer(true);
        }

        Cursor cursorToRender() {
            ASSERT(attached());
            return rootWindow_->cursor();
        }

        RootWindow const * rootWindow() const {
            return rootWindow_;
        }

        RootWindow * rootWindow() {
            return rootWindow_;
        }

        void setRootWindow(RootWindow * rootWindow) {
            detach();
            attach(rootWindow);
        }

        bool attached() const {
            return rootWindow_ != nullptr;
        }

        void updateSize(int cols, int rows) {
            if (attached())
                rootWindow_->rendererResized(cols, rows);
        }

        void setFocus(bool value) {
            if (attached())
                rootWindow_->rendererFocused(value);
        }

        /** Mouse events. 
         */
        void mouseDown(int col, int row, MouseButton button, Key modifiers) {
            if (attached())
                rootWindow_->mouseDown(col, row, button, modifiers);
        }

        void mouseUp(int col, int row, MouseButton button, Key modifiers) {
            if (attached())
                rootWindow_->mouseUp(col, row, button, modifiers);
        }

        void mouseWheel(int col, int row, int by, Key modifiers) {
            if (attached())
                rootWindow_->mouseWheel(col, row, by, modifiers);
        }

        void mouseMove(int col, int row, Key modifiers) {
            if (attached())
                rootWindow_->mouseMove(col, row, modifiers);
        }

        void mouseEnter() {
            if (attached())
                rootWindow_->mouseEnter();
        }

        void mouseLeave() {
            if (attached())
                rootWindow_->mouseLeave();
        }

        /** Keyboard events. 
         */
        void keyChar(helpers::Char c) {
            if (attached())
                rootWindow_->keyChar(c);
        }

        void keyDown(Key k) {
            if (attached())
                rootWindow_->keyDown(k);
        }

        void keyUp(Key k) {
            if (attached())
                rootWindow_->keyUp(k);
        }

        // clipboard events

        void paste(std::string const & contents) {
            if (attached())
                rootWindow_->paste(contents);
        }

        void selectionInvalidated() {
            if (attached())
                rootWindow_->selectionInvalidated(); 
        }

    private:

        virtual void attach(RootWindow * newRootWindow);

        virtual void detach();

        RootWindow * rootWindow_;

    }; // ui::Renderer

} // namespace ui
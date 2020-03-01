#pragma once

#ifndef NDEBUG
#include <thread>
#include <mutex>
#endif

#include "common.h"
#include "buffer.h"
#include "widget.h"

namespace ui2 {


    /** UI Renderer
     
        Class responsible for rendering the widgets and providing the user actions such as keyboard, mouse and selection & clipboard.
     */
    class Renderer {
    public:

        std::string const & title() const {
            return title_;
        }

        void setTitle(std::string const & value) {
            UI_THREAD_CHECK;
            if (title_ != value) {
                title_ = value;
                titleChanged();
            }
        }

        /** Requests repaint of the given widget. [thread-safe]
         
            The purpose of this method is to use whatever event queue (or other mechanism) the target rendering supports to schedule repaint of the specified widget in the main UI thread. 
         */
        virtual void repaint(Widget * widget) = 0;

    protected:

        friend class Canvas;
        friend class Widget;

        /** Called when the selected rectangle of the backing buffer has been updated and needs rendered. 
         
            This method should actually render the contents. While only the refresh of the given rectangle is necessary at the time the method is called, the entire buffer is guaranteed to be valid in case whole screen repaint is to be issued. 
         */
        virtual void render(Rect const & rect) = 0;

        /** Called when the title of the renderer has changed. 
         */
        virtual void titleChanged() = 0;

        /** \name Mouse Input 
         
         */
        //@{
        virtual void mouseDown(Point coords, MouseButton button, Key modifiers) {
            NOT_IMPLEMENTED;
        }

        virtual void mouseUp(Point coords, MouseButton button, Key modifiers) {
            NOT_IMPLEMENTED;
        }

        virtual void mouseClick(Point coords, MouseButton button, Key modifiers) {
            NOT_IMPLEMENTED;
        }

        virtual void mouseDoubleClick(Point coords, MouseButton button, Key modifiers) {
            NOT_IMPLEMENTED;
        }

        virtual void mouseWheel() {
            NOT_IMPLEMENTED;
        }

        virtual void mouseMove() {
            NOT_IMPLEMENTED;
        }

        virtual void mouseEnter() {
            NOT_IMPLEMENTED;
        }

        virtual void mouseLeave() {
            NOT_IMPLEMENTED;
        }

        //@}

        /** \name Keyboard Input
         
            - who is going to keep track of modal windows and other things? 
         */
        //@{
        virtual void keyChar(Char c) {
            if (keyboardFocus_ == nullptr)
                return;
            Event<Char>::Payload payload{c};
            keyboardFocus_->keyChar(payload);
        }

        virtual void keyDown(Key k) {
            if (keyboardFocus_ == nullptr)
                return;
            Event<Key>::Payload payload{k};
            keyboardFocus_->keyDown(payload);
        }

        virtual void keyUp(Key k) {
            if (keyboardFocus_ == nullptr)
                return;
            Event<Key>::Payload payload{k};
            keyboardFocus_->keyUp(payload);
        }

        //@}

        /** \name Clipboard & Selection
         */
        //@{
        virtual void paste() {
            NOT_IMPLEMENTED;
        }
        //@}

        /** Called when a widget is attached to the renderer. 
         
            When attaching a subtree, the widgetAttached method will be called for each widget in the subtree. Called *after* the widget's renderer has been set. 
         */
        virtual void widgetAttached(Widget * widget) {
            UI_THREAD_CHECK;
            // TODO do nothing ? or make abstract?
        }

        /** Called when a widget is detached (removed from the tree).
         
            If a subtree is removed, the method is called for each widget starting at the leaves. At the call the widget is still attached. 
         */
        virtual void widgetDetached(Widget * widget) {
            UI_THREAD_CHECK;
            // TODO do nothing ? or make abstract?
        }

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

        /** Renderer's window title. 
         */
        std::string title_;

        Widget * keyboardFocus_;



#ifndef NDEBUG
        friend class UiThreadChecker_;
        
        Renderer * getRenderer_() const {
            return const_cast<Renderer*>(this);
        }

        std::thread::id uiThreadId_;
        size_t uiThreadChecksDepth_;
        std::mutex uiThreadCheckMutex_;
#endif

    }; // ui::Renderer


} // namespace ui
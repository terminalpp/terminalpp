#pragma once

#include "helpers/helpers.h"

#include "widget.h"

namespace ui {

    class Renderer;

    /** Base class for all object that can be rendered using the Renderer interface.
     */
    class Renderable {
    protected:

        Renderable():
            renderer_(nullptr) {
        }

        virtual ~Renderable();

        /** Obtains the buffer to be rendered. 
         */
        virtual Canvas::Buffer::Ptr buffer(bool priority) = 0;

        virtual Cursor const & cursor() const = 0;

        /** Called by the attached renderer when its size changes. 
         */
        virtual void rendererResized(int cols, int rows) = 0;

        virtual void rendererFocused(bool value) = 0;

        /** Mouse events. 
         */
        virtual void mouseDown(int col, int row, MouseButton button, Key modifiers) = 0;
        virtual void mouseUp(int col, int row, MouseButton button, Key modifiers) = 0;
        virtual void mouseWheel(int col, int row, int by, Key modifiers) = 0;
        virtual void mouseMove(int col, int row, Key modifiers) = 0;

        /** Keyboard events. 
         */
        virtual void keyChar(helpers::Char c) = 0;
        virtual void keyDown(Key k) = 0;
        virtual void keyUp(Key k) = 0;

        /** Clipboard events. 
         */
        virtual void paste(std::string const & contents) = 0;

        Renderer const * renderer() const {
            return renderer_;
        }

        Renderer * renderer() {
            return renderer_;
        }

        bool attached() const {
            return renderer_ != nullptr;
        }

        void render(Rect const & rect);

        void setClipboard(std::string const & contents);

        void setSelection(std::string const & contents);

        void requestClipboardPaste();

        void requestSelectionPaste();

    private:
        friend class Renderer;

        Renderer * renderer_;

        Widget * pasteSender_;
    }; // ui::Renderable

    /** Base class for user interface renderers.
     */
    class Renderer {
    protected:
        friend class Renderable;

        Renderer():
            renderable_(nullptr) {
        }

        virtual ~Renderer() {
            detach();
        }

        virtual int cols() const = 0;
        virtual int rows() const = 0;

        /** Paints the window, or its part. 
         * */
        virtual void render(ui::Rect const & rect) = 0;

        /** Sets the contents of the clipboard to the given text. 
         */
        virtual void setClipboard(std::string const & contents) = 0;

        /** Sets the selection to given value. 
         */
        virtual void setSelection(std::string const & contents) = 0;

        virtual void invalidateSelection() = 0;

        virtual void requestSelectionPaste() = 0;
        virtual void requestClipboardPaste() = 0;

        Canvas::Buffer::Ptr bufferToRender() {
            ASSERT(renderable_);
            return renderable_->buffer(true);
        }

        Cursor cursorToRender() {
            ASSERT(renderable_);
            return renderable_->cursor();
        }

        Renderable const * renderable() const {
            return renderable_;
        }

        Renderable * renderable() {
            return renderable_;
        }

        void setRenderable(Renderable * renderable) {
            detach();
            attach(renderable);
        }

        bool attached() const {
            return renderable_ != nullptr;
        }

        void updateSize(int cols, int rows) {
            if (attached())
                renderable_->rendererResized(cols, rows);
        }

        void setFocus(bool value) {
            if (attached())
                renderable_->rendererFocused(value);
        }

        /** Mouse events. 
         */
        void mouseDown(int col, int row, MouseButton button, Key modifiers) {
            if (attached())
                renderable_->mouseDown(col, row, button, modifiers);
        }

        void mouseUp(int col, int row, MouseButton button, Key modifiers) {
            if (attached())
                renderable_->mouseUp(col, row, button, modifiers);
        }

        void mouseWheel(int col, int row, int by, Key modifiers) {
            if (attached())
                renderable_->mouseWheel(col, row, by, modifiers);
        }

        void mouseMove(int col, int row, Key modifiers) {
            if (attached())
                renderable_->mouseMove(col, row, modifiers);
        }

        /** Keyboard events. 
         */
        void keyChar(helpers::Char c) {
            if (attached())
                renderable_->keyChar(c);
        }

        void keyDown(Key k) {
            if (attached())
                renderable_->keyDown(k);
        }

        void keyUp(Key k) {
            if (attached())
                renderable_->keyUp(k);
        }

        /* Clipboard events.
         */
        void paste(std::string const & contents) {
            if (attached())
                renderable_->paste(contents);
        }


    private:

        virtual void attach(Renderable * renderable) {
            ASSERT(renderable_ == nullptr);
            ASSERT(renderable->renderer_ == nullptr);
            renderable_ = renderable;
            renderable_->renderer_ = this;
            renderable_->rendererResized(cols(), rows());
        }

        virtual void detach() {
            if (! renderable_)
                return;
            renderable_->renderer_ = nullptr;
            renderable_ = nullptr;
        }

        Renderable * renderable_;

    }; // ui::Renderer

    inline Renderable::~Renderable() {
        if (attached())
            renderer_->detach();
    }

    inline void Renderable::render(Rect const & rect) {
        if (attached())
            renderer_->render(rect);
    }

    inline void Renderable::setClipboard(std::string const & contents) {
        if (attached())
            renderer_->setClipboard(contents);
    }

    inline void Renderable::setSelection(std::string const & contents) {
        if (attached())
            renderer_->setSelection(contents);
    }

    inline void Renderable::requestClipboardPaste() {
        if (attached())
            renderer_->requestClipboardPaste();
    }

    inline void Renderable::requestSelectionPaste() {
        if (attached())
            renderer_->requestSelectionPaste();
    }

} // namespace ui
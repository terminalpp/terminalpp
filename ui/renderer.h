#pragma once

#include "helpers/helpers.h"

#include "widget.h"
#include "clipboard.h"

namespace ui {

    class Renderer;

    /** Base class for all object that can be rendered using the Renderer interface.
     */
    class Renderable {
    protected:

        friend class Clipboard;

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

        // clipboard

        void requestClipboardPaste(Clipboard * sender);

        void requestSelectionPaste(Clipboard * sender);

        void setClipboard(Clipboard * sender, std::string const & contents);

        void setSelection(Clipboard * sender, std::string const & contents);

        void clearSelection(Clipboard * sender);


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


        virtual void requestClipboardPaste(Clipboard * sender) = 0;

        virtual void requestSelectionPaste(Clipboard * sender) = 0;

        /** Sets the contents of the clipboard to the given text. 
         */
        virtual void setClipboard(Clipboard * sender, std::string const & contents) = 0;

        /** Sets the selection to given value. 
         */
        virtual void setSelection(Clipboard * sender, std::string const & contents) = 0;

        virtual void clearSelection(Clipboard * sender) = 0;

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

        // clipboard events

        void paste(Clipboard * clipboard, std::string const & contents) {
            clipboard->paste(contents);
        }

        void invalidateSelection(Clipboard * sender, Clipboard * currentOwner) {
            if (sender == currentOwner)
                return;
            if (currentOwner != nullptr)
                currentOwner->invalidateSelection();
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

    inline void Renderable::requestClipboardPaste(Clipboard * sender) {
        if (attached())
            renderer_->requestClipboardPaste(sender);
    }

    inline void Renderable::requestSelectionPaste(Clipboard * sender) {
        if (attached())
            renderer_->requestSelectionPaste(sender);
    }

    inline void Renderable::setClipboard(Clipboard * sender, std::string const & contents) {
        if (attached())
            renderer_->setSelection(sender, contents);
    }

    inline void Renderable::setSelection(Clipboard * sender, std::string const & contents) {
        if (attached())
            renderer_->setSelection(sender, contents);
    }

    inline void Renderable::clearSelection(Clipboard * sender) {
        if (attached())
            renderer_->clearSelection(sender);
    }

} // namespace ui
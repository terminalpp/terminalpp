#pragma once
#if (defined RENDERER_QT)

#include <QtWidgets>

#include "qt_font.h"
#include "../window.h"

namespace tpp {

    class QtWindow : public RendererWindow<QtWindow, QWindow *> {
    public:

        ~QtWindow() override;

        void show() override;

        void hide() override {
            NOT_IMPLEMENTED;
        }

        void requestClose() override {
            NOT_IMPLEMENTED;
        }

        /** Sets the title of the window. 
         */
		void requestRender(ui::Rect const & rect) override {
            MARK_AS_UNUSED(rect);
            NOT_IMPLEMENTED;
        }

        void setTitle(std::string const & title) override {
            MARK_AS_UNUSED(title);
            NOT_IMPLEMENTED;
        }

		/** Sets the window icon. 
		 */
        void setIcon(ui::RootWindow::Icon icon) override;

    protected:

        friend class QtApplication;

        typedef RendererWindow<QtWindow, QWindow *> Super;


        QtWindow(std::string const & title, int cols, int rows, unsigned baseCellHeightPx);

        void requestClipboardContents() override;

        void requestSelectionContents() override;

        void setClipboard(std::string const & contents) override;

        void setSelection(std::string const & contents) override;

        void clearSelection() override;


    private:

        friend class RendererWindow<QtWindow, QWindow *>;

        // rendering methods - we really want these to inline

        void initializeDraw() {
        }

        void finalizeDraw() {
        }

        void initializeGlyphRun(int col, int row) {
            MARK_AS_UNUSED(col);
            MARK_AS_UNUSED(row);
        }

        void addGlyph(int col, int row, ui::Cell const & cell) {
            MARK_AS_UNUSED(col);
            MARK_AS_UNUSED(row);
            MARK_AS_UNUSED(cell);
        }

        /** Updates the current font.
         */
        void setFont(ui::Font font) {
            MARK_AS_UNUSED(font);
        }

        /** Updates the foreground color.
         */
        void setForegroundColor(ui::Color color) {
            MARK_AS_UNUSED(color);
        }

        /** Updates the background color. 
         */
        void setBackgroundColor(ui::Color color) {
            MARK_AS_UNUSED(color);
        }

        /** Updates the decoration color. 
         */
        void setDecorationColor(ui::Color color) {
            MARK_AS_UNUSED(color);
        }
    
        /** Updates the decoration color. 
         */
        void setBorderColor(ui::Color color) {
            MARK_AS_UNUSED(color);
        }

        /** Sets the attributes of the cell. 
         */
        void setAttributes(ui::Attributes const & attrs) {
            MARK_AS_UNUSED(attrs);
        }

        /** Draws the glyph run. 
         
            First clears the background with given background color, then draws the text and finally applies any decorations. 
         */
        void drawGlyphRun() {
        }

        void drawBorder(ui::Attributes attrs, int left, int top, int width) {
            MARK_AS_UNUSED(attrs);
            MARK_AS_UNUSED(left);
            MARK_AS_UNUSED(top);
            MARK_AS_UNUSED(width);
        }

    }; // QtWindow

} // namespace tpp

#endif
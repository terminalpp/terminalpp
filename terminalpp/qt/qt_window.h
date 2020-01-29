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
        }

        void setTitle(std::string const & title) override {
        }

		/** Sets the window icon. 
		 */
        void setIcon(ui::RootWindow::Icon icon) override;

    protected:
        QtWindow(std::string const & title, int cols, int rows, unsigned baseCellHeightPx);

        // rendering methods - we really want these to inline

        void initializeDraw() {
        }

        void finalizeDraw() {
        }

        void initializeGlyphRun(int col, int row) {
        }

        void addGlyph(int col, int row, ui::Cell const & cell) {
        }

        /** Updates the current font.
         */
        void setFont(ui::Font font) {
        }

        /** Updates the foreground color.
         */
        void setForegroundColor(ui::Color color) {
        }

        /** Updates the background color. 
         */
        void setBackgroundColor(ui::Color color) {
        }

        /** Updates the decoration color. 
         */
        void setDecorationColor(ui::Color color) {
        }
    
        /** Updates the decoration color. 
         */
        void setBorderColor(ui::Color color) {
        }

        /** Sets the attributes of the cell. 
         */
        void setAttributes(ui::Attributes const & attrs) {
        }

        /** Draws the glyph run. 
         
            First clears the background with given background color, then draws the text and finally applies any decorations. 
         */
        void drawGlyphRun() {
        }

        void drawBorder(ui::Attributes attrs, int left, int top, int width) {
        }

    }; // QtWindow

} // namespace tpp

#endif
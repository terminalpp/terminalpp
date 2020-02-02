#pragma once
#if (defined RENDERER_QT)

#include <QtWidgets>

#include "qt_font.h"
#include "../window.h"

namespace tpp {

    /**
     */
    // QT object must be the first base, otherwise teh code won't compile
    class QtWindow : public QRasterWindow, public RendererWindow<QtWindow, QWindow *> {
        Q_OBJECT
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

            TODO perhaps the signal must be marked as thread safe or something?  
         */
		void requestRender(ui::Rect const & rect) override {
            MARK_AS_UNUSED(rect);
            emit tppRequestUpdate();
            //QtApplication::Instance()->postEvent(this, new QPaintEvent{QRect{0,0, widthPx_, heightPx_}});
        }

        void setTitle(std::string const & title) override {
            QRasterWindow::setTitle(title.c_str());
        }

		/** Sets the window icon. 
		 */
        void setIcon(ui::RootWindow::Icon icon) override;

    signals:

        void tppRequestUpdate();

    protected:

        friend class QtApplication;

        typedef RendererWindow<QtWindow, QWindow *> Super;


        QtWindow(std::string const & title, int cols, int rows, unsigned baseCellHeightPx);

        /** Qt's paint event just delegates to the paint method of the RendererWindow. 
         */
        void paintEvent(QPaintEvent * e) override {
            MARK_AS_UNUSED(e);
            Super::paint();
        }

        void updateSizePx(unsigned widthPx, unsigned heightPx) override {
            QRasterWindow::resize(widthPx, heightPx);
            Super::updateSizePx(widthPx, heightPx);
            repaint();
        }

        void updateSize(int cols, int rows) override {
            Super::updateSize(cols, rows);
            repaint();    
        }

        void requestClipboardContents() override;

        void requestSelectionContents() override;

        void setClipboard(std::string const & contents) override;

        void setSelection(std::string const & contents) override;

        void clearSelection() override;


    private:

        friend class RendererWindow<QtWindow, QWindow *>;

        // rendering methods - we really want these to inline

        void initializeDraw() {
            painter_.begin(this);
        }

        void finalizeDraw() {
            painter_.end();
        }

        void initializeGlyphRun(int col, int row) {
            MARK_AS_UNUSED(col);
            MARK_AS_UNUSED(row);
        }

        void addGlyph(int col, int row, ui::Cell const & cell) {
            char32_t cp{ cell.codepoint() };
            painter_.drawText(col * cellWidthPx_, (row + 1) * cellHeightPx_, QString::fromUcs4(&cp, 1));
        }

        /** Updates the current font.
         */
        void setFont(ui::Font font) {
            font_ = QtFont::GetOrCreate(font, cellWidthPx_, cellHeightPx_);
            painter_.setFont(font_->font());
        }

        /** Updates the foreground color.
         */
        void setForegroundColor(ui::Color color) {
            painter_.setPen(QColor{ color.r, color.g, color.b, color.a });
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

        QPainter painter_;
        QtFont * font_;

    }; // QtWindow

} // namespace tpp

#endif
#pragma once
#if (defined RENDERER_QT)

#include <QtWidgets>

#include "qt_font.h"
#include "../window.h"

namespace tpp {

    typedef QOpenGLWindow QWindowBase;

    /**
     
        TODO after QT rendering done:

        - rename Fullscreen to FullScreen
        - see if zoom can be done at the Window level
        - qt rendering is not really fast (only render what needs to be rendered?)
        

     */
    // QT object must be the first base, otherwise teh code won't compile
    class QtWindow : public QWindowBase, public RendererWindow<QtWindow, QWindow *> {
        Q_OBJECT
    public:

        ~QtWindow() override;

        void show() override;

        void hide() override {
            NOT_IMPLEMENTED;
        }

        void requestClose() override {
            emit tppWindowClose();
        }

        /** Sets the title of the window. 
         */
		void requestRender(ui::Rect const & rect) override {
            MARK_AS_UNUSED(rect);
            emit tppRequestUpdate();
        }

        void setTitle(std::string const & title) override {
            emit tppSetTitle(QString{title.c_str()});
        }

        using QWindowBase::setTitle;

		/** Sets the window icon. 
		 */
        // TODO this has to be made thread safe
        void setIcon(ui::RootWindow::Icon icon) override;

    signals:

        void tppRequestUpdate();
        void tppShowFullScreen();
        void tppShowNormal();
        void tppWindowClose();
        void tppSetTitle(QString const &);

        void tppSetClipboard(QString);
        void tppSetSelection(QString, QtWindow *);
        void tppClearSelection(QtWindow *);

        void tppGetClipboard(QtWindow *);
        void tppGetSelection(QtWindow *);

    protected:

        friend class QtApplication;

        typedef RendererWindow<QtWindow, QWindow *> Super;


        QtWindow(std::string const & title, int cols, int rows, unsigned baseCellHeightPx);

        /** \name QT events.
         */
        //@{

        /** Qt's paint event just delegates to the paint method of the RendererWindow. 
         */
        void paintEvent(QPaintEvent * ev) override {
            MARK_AS_UNUSED(ev);
            Super::paint();
        }

        /** Qt's resize event.
         */
        void resizeEvent(QResizeEvent* ev) override {
            QWindowBase::resizeEvent(ev);
            updateSizePx(ev->size().width(), ev->size().height());
        }

        /** Key press event. 
         */
        void keyPressEvent(QKeyEvent * ev) override;

        /** Key release event.
         */
        void keyReleaseEvent(QKeyEvent * ev) override;

        void mousePressEvent(QMouseEvent * ev) override;

        void mouseReleaseEvent(QMouseEvent * ev) override;

        void mouseMoveEvent(QMouseEvent * ev) override;

        void wheelEvent(QWheelEvent * ev) override;

        //@}

        void updateSizePx(unsigned widthPx, unsigned heightPx) override {
            Super::updateSizePx(widthPx, heightPx);
            repaint();
        }

        void updateSize(int cols, int rows) override {
            Super::updateSize(cols, rows);
            repaint();    
        }

        void updateFullscreen(bool value) override {
            if (value)
                emit tppShowFullScreen();
            else
                emit tppShowNormal();
            Super::updateFullscreen(value);
        }

        void updateZoom(double value) override {
            QtFont * f = QtFont::GetOrCreate(ui::Font(), 0, static_cast<unsigned>(baseCellHeightPx_ * value));
            cellWidthPx_ = f->widthPx();
            cellHeightPx_ = f->heightPx();
            Super::updateZoom(value);
            Super::updateSizePx(widthPx_, heightPx_);
        }

        void requestClipboardContents() override {
            emit tppGetClipboard(this);
        }

        void requestSelectionContents() override {
            emit tppGetSelection(this);
        }

        void setClipboard(std::string const & contents) override {
            emit tppSetClipboard(QString{contents.c_str()});
        }

        void setSelection(std::string const & contents) override {
            emit tppSetSelection(QString{contents.c_str()}, this);
        }

        void clearSelection() override {
            emit tppClearSelection(this);
        }


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
            painter_.fillRect(QRect{col * cellWidthPx_, row * cellHeightPx_, cellWidthPx_ * statusCell_.font().width(), cellHeightPx_ * statusCell_.font().height()}, painter_.brush());
            painter_.drawText(col * cellWidthPx_, (row + 1 - statusCell_.font().height()) * cellHeightPx_ + font_->ascent(), QString::fromUcs4(&cp, 1));
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
            painter_.setBrush(QColor{ color.r, color.g, color.b, color.a });
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

        static unsigned GetStateModifiers(Qt::KeyboardModifiers const & modifiers);

        static ui::Key GetKey(int qtKey, unsigned modifiers, bool pressed);

        QPainter painter_;
        QtFont * font_;

    }; // QtWindow

} // namespace tpp

#endif
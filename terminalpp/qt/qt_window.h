#pragma once
#if (defined RENDERER_QT)

#include <QtWidgets>

#include "qt_font.h"
#include "../window.h"

namespace tpp2 {

    typedef QOpenGLWindow QWindowBase;

    // QT object must be the first base, otherwise teh code won't compile
    class QtWindow : public QWindowBase, public RendererWindow<QtWindow, QWindow *> {
        Q_OBJECT
    public:

        ~QtWindow() override;

        void repaint(Widget * widget) {
            emit tppRequestUpdate();
        };

        void setTitle(std::string const & value) override;

        void setIcon(Window::Icon icon) override;

        void setFullscreen(bool value = true) override {
            if (value)
                emit tppShowFullScreen();
            else
                emit tppShowNormal();
            Super::setFullscreen(value);
        }

        void show(bool value = true) override;

        /*
        void resize(int newWidth, int newHeight) override {
            if (newWidth != width())
                updateXftStructures(newWidth);
            RendererWindow::resize(newWidth, newHeight);
        }
        */

    signals:
       void tppRequestUpdate();
       void tppShowFullScreen();
       void tppShowNormal();
       void tppWindowClose();

    protected:

        /*
        void windowResized(int width, int height) override {
			if (buffer_ != 0) {
				XFreePixmap(display_, buffer_);
				buffer_ = 0;
			}
            RendererWindow::windowResized(width, height);
        }
        */


        /** Destroys the renderer's window. 
         */
        void rendererClose() override {
            emit tppWindowClose();
        }

    private:
        friend class QtApplication;
        friend class RendererWindow<QtWindow, QWindow*>;
        typedef RendererWindow<QtWindow, QWindow *> Super;

        /** Creates the renderer window of appropriate size using the default font and zoom of 1.0. 
         */
        QtWindow(std::string const & title, int cols, int rows);

        /** Qt's resize event.
         */
        void resizeEvent(QResizeEvent* ev) override {
            QWindowBase::resizeEvent(ev);
            windowResized(ev->size().width(), ev->size().height());
        }

        /** \name Input handling. 
         */
        //@{
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

        void focusInEvent(QFocusEvent * ev) override;

        void focusOutEvent(QFocusEvent * ev) override;

        //@}


        /** \name Rendering Functions
         */
        //@{
        /** Qt's paint event just delegates to the paint method of the RendererWindow. 
         */
        void paintEvent(QPaintEvent * ev) override {
            MARK_AS_UNUSED(ev);
            render(rootWidget());
        }
        void initializeDraw() {
            painter_.begin(this);
        }

        void finalizeDraw() {
            changeBackgroundColor(backgroundColor());
            if (widthPx_ % cellWidth_ != 0)
                painter_.fillRect(QRect{Super::width() * cellWidth_, 0, widthPx_ % cellWidth_, heightPx_}, painter_.brush());
            if (heightPx_ % cellHeight_ != 0)
                painter_.fillRect(QRect{0, Super::height() * cellHeight_, widthPx_, heightPx_ % cellHeight_}, painter_.brush());
            painter_.end();
        }

        void initializeGlyphRun(int col, int row) {
            glyphRunStart_ = Point{col, row};
            glyphRunSize_ = 0;
        }

        void addGlyph(int col, int row, Cell const & cell) {
            char32_t cp{ cell.codepoint() };
            int fontWidth = state_.font().width();
            int fontHeight = state_.font().height();
            if (state_.bg().a != 0) {
                painter_.fillRect(col * cellWidth_, (row + 1 - fontHeight)  * cellHeight_, cellWidth_ * fontWidth, cellHeight_ * fontHeight, painter_.brush());   
            }
            if ((cp != 32) && (!state_.font().blink() || BlinkVisible_)) {
                painter_.drawText(col * cellWidth_, (row + 1 - state_.font().height()) * cellHeight_ + font_->ascent(), QString::fromUcs4(&cp, 1));
            }
            ++glyphRunSize_;
        }

        /** Updates the current font.
         */
        void changeFont(ui2::Font font) {
            font_ = QtFont::Get(font, cellHeight_, cellWidth_);
            painter_.setFont(font_->qFont());
        }

        /** Updates the foreground color.
         */
        void changeForegroundColor(Color color) {
            painter_.setPen(QColor{ color.r, color.g, color.b, color.a });
        }

        /** Updates the background color. 
         */
        void changeBackgroundColor(Color color) {
            painter_.setBrush(QColor{ color.r, color.g, color.b, color.a });
        }

        /** Updates the decoration color. 
         */
        void changeDecorationColor(Color color) {
            decorationBrush_.setColor(QColor{ color.r, color.g, color.b, color.a });
        }

        /** Draws the glyph run. 
         
            Since Qt glyphs are drawn one by one in the addGlyph function, what remains to be done in drawGlyph run is to draw the underline or strikethrough decorations. 
         */
        void drawGlyphRun() {
            if (glyphRunSize_ == 0)
                return;
            if (!state_.font().blink() || BlinkVisible_) {
                if (state_.font().underline())
                    painter_.fillRect(glyphRunStart_.x() * cellWidth_, glyphRunStart_.y() * cellHeight_ + font_->underlineOffset(), cellWidth_ * glyphRunSize_, font_->underlineThickness(), decorationBrush_);
                if (state_.font().strikethrough())
                    painter_.fillRect(glyphRunStart_.x() * cellWidth_, glyphRunStart_.y() * cellHeight_ + font_->strikethroughOffset(), cellWidth_ * glyphRunSize_, font_->strikethroughThickness(), decorationBrush_);
            }
            glyphRunStart_ += Point{glyphRunSize_, 0};
            glyphRunSize_ = 0;
        }

        void drawBorder(int col, int row, Border const & border, int widthThin, int widthThick) {
            int left = col * cellWidth_;
            int top = row * cellHeight_;
            int widthTop = border.top() == Border::Kind::None ? 0 : (border.top() == Border::Kind::Thick ? widthThick : widthThin);
            int widthLeft = border.left() == Border::Kind::None ? 0 : (border.left() == Border::Kind::Thick ? widthThick : widthThin);
            int widthBottom = border.bottom() == Border::Kind::None ? 0 : (border.bottom() == Border::Kind::Thick ? widthThick : widthThin);
            int widthRight = border.right() == Border::Kind::None ? 0 : (border.right() == Border::Kind::Thick ? widthThick : widthThin);

            if (widthTop != 0)
                painter_.fillRect(left, top, cellWidth_, widthTop, painter_.brush());
            if (widthBottom != 0)
                painter_.fillRect(left, top + cellHeight_ - widthBottom, cellWidth_, widthBottom, painter_.brush());
            if (widthLeft != 0) 
                painter_.fillRect(left, top + widthTop, widthLeft, cellHeight_ - widthTop - widthBottom, painter_.brush());
            if (widthRight != 0) 
                painter_.fillRect(left + cellWidth_ - widthRight, top + widthTop, widthRight, cellHeight_ - widthTop - widthBottom, painter_.brush());
        }
        //@}

        static unsigned GetStateModifiers(Qt::KeyboardModifiers const & modifiers);
        static Key GetKey(int qtKey, unsigned modifiers, bool pressed);

        QPainter painter_;
        QtFont * font_;

        QBrush decorationBrush_;
        QBrush borderBrush_;

        Point glyphRunStart_;
        int glyphRunSize_;

    }; // tpp::QtWindow



} // namespace tpp




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

		/** Sets the window icon. 
		 */
        void setIcon(ui::RootWindow::Icon icon) override;

        using QWindowBase::setTitle;
        using QWindowBase::setIcon;

    signals:

        void tppRequestUpdate();
        void tppShowFullScreen();
        void tppShowNormal();
        void tppWindowClose();
        void tppSetTitle(QString const &);
        void tppSetIcon(QIcon const &);

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

        void focusInEvent(QFocusEvent * ev) override;

        void focusOutEvent(QFocusEvent * ev) override;

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
            setBackgroundColor(rootWindow()->backgroundColor());
            if (widthPx_ % cellWidthPx_ != 0)
                painter_.fillRect(QRect{cols_ * cellWidthPx_, 0, widthPx_ % cellWidthPx_, heightPx_}, painter_.brush());
            if (heightPx_ % cellHeightPx_ != 0)
                painter_.fillRect(QRect{0, rows_ * cellHeightPx_, widthPx_, heightPx_ % cellHeightPx_}, painter_.brush());
            painter_.end();
        }

        void initializeGlyphRun(int col, int row) {
            glyphRunStart_ = ui::Point{col, row};
            glyphRunSize_ = 0;
        }

        void addGlyph(int col, int row, ui::Cell const & cell) {
            char32_t cp{ cell.codepoint() };
            int fontWidth = statusCell_.font().width();
            int fontHeight = statusCell_.font().height();
            if (statusCell_.bg().a != 0) {
                painter_.fillRect(col * cellWidthPx_, (row + 1 - fontHeight)  * cellHeightPx_, cellWidthPx_ * fontWidth, cellHeightPx_ * fontHeight, painter_.brush());   
            }
            if ((cp != 32) && (!statusCell_.attributes().blink() || blinkVisible_)) {
                painter_.drawText(col * cellWidthPx_, (row + 1 - statusCell_.font().height()) * cellHeightPx_ + font_->ascent(), QString::fromUcs4(&cp, 1));
            }
            ++glyphRunSize_;
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
            decorationBrush_.setColor(QColor{ color.r, color.g, color.b, color.a });
        }
    
        /** Updates the decoration color. 
         */
        void setBorderColor(ui::Color color) {
            borderBrush_.setColor(QColor{ color.r, color.g, color.b, color.a });
        }

        /** Sets the attributes of the cell. 
         */
        void setAttributes(ui::Attributes const & attrs) {
            MARK_AS_UNUSED(attrs);
        }

        /** Draws the glyph run. 
         */
        void drawGlyphRun() {
            if (glyphRunSize_ == 0)
                return;
            if (!statusCell_.attributes().emptyDecorations() && (!statusCell_.attributes().blink() || blinkVisible_)) {
                if (statusCell_.attributes().underline())
                    painter_.fillRect(glyphRunStart_.x * cellWidthPx_, glyphRunStart_.y * cellHeightPx_ + font_->underlineOffset(), cellWidthPx_ * glyphRunSize_, font_->underlineThickness(), decorationBrush_);
                if (statusCell_.attributes().strikethrough())
                    painter_.fillRect(glyphRunStart_.x * cellWidthPx_, glyphRunStart_.y * cellHeightPx_ + font_->strikethroughOffset(), cellWidthPx_ * glyphRunSize_, font_->strikethroughThickness(), decorationBrush_);
            }
        }

        void drawBorder(ui::Attributes attrs, int left, int top, int width) {
            int cLeft = left;
            int cTop = top;
            int cWidth = cellWidthPx_;
            int cHeight = width;
            // if top border is selected, draw the top line 
            if (attrs.borderTop()) {
                painter_.fillRect(cLeft, cTop, cWidth, cHeight, borderBrush_);
            // otherwise see if the left or right parts of the border should be drawn
            } else {
                if (attrs.borderLeft()) {
                    cWidth = width;
                    painter_.fillRect(cLeft, cTop, cWidth, cHeight, borderBrush_);
                }
                if (attrs.borderRight()) {
                    cWidth = width;
                    cLeft = left + cellWidthPx_ - width;
                    painter_.fillRect(cLeft, cTop, cWidth, cHeight, borderBrush_);
                }
            }
            // check the left and right border in the middle part
            cTop += width;
            cHeight = cellHeightPx_ - 2 * width;
            if (attrs.borderLeft()) {
                cLeft = left;
                cWidth = width;
                painter_.fillRect(cLeft, cTop, cWidth, cHeight, borderBrush_);
            }
            if (attrs.borderRight()) {
                cWidth = width;
                cLeft = left + cellWidthPx_ - width;
                painter_.fillRect(cLeft, cTop, cWidth, cHeight, borderBrush_);
            }
            // check if the bottom part should be drawn, first by checking whether the whole bottom part should be drawn, if not then by separate checking the left and right corner
            cTop = top + cellHeightPx_ - width;
            cHeight = width;
            if (attrs.borderBottom()) {
                cLeft = left;
                cWidth = cellWidthPx_;
                painter_.fillRect(cLeft, cTop, cWidth, cHeight, borderBrush_);
            } else {
                if (attrs.borderLeft()) {
                    cLeft = left;
                    cWidth = width;
                    painter_.fillRect(cLeft, cTop, cWidth, cHeight, borderBrush_);
                }
                if (attrs.borderRight()) {
                    cWidth = width;
                    cLeft = left + cellWidthPx_ - width;
                    painter_.fillRect(cLeft, cTop, cWidth, cHeight, borderBrush_);
                }
            }
        }

        static unsigned GetStateModifiers(Qt::KeyboardModifiers const & modifiers);

        static ui::Key GetKey(int qtKey, unsigned modifiers, bool pressed);

        QPainter painter_;
        QtFont * font_;

        QBrush decorationBrush_;
        QBrush borderBrush_;

        ui::Point glyphRunStart_;
        int glyphRunSize_;

    }; // QtWindow

} // namespace tpp

#endif
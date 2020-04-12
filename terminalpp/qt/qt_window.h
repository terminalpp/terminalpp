#pragma once
#if (defined RENDERER_QT)

#include <QtWidgets>

#include "qt_font.h"
#include "../window.h"

namespace tpp {

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

        void requestClipboard(Widget * sender) override;

        void requestSelection(Widget * sender) override;

        void rendererSetClipboard(std::string const & contents) override;

        void rendererRegisterSelection(std::string const & contents, Widget * owner) override;

        void rendererClearSelection() override;

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

#endif
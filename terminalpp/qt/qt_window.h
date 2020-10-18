#pragma once
#if (defined RENDERER_QT)

#include <QtWidgets>

#include "qt_font.h"
#include "../window.h"

namespace tpp {

    // QT object must be the first base, otherwise teh code won't compile
    class QtWindow : public QWidget, public RendererWindow<QtWindow, QWidget *> {
        Q_OBJECT
    public:
        using RendererWindow<QtWindow, QWidget *>::repaint;

        /** Bring the font to the class' namespace so that the RendererWindow can find it. 
         */
        using Font = QtFont;

        ~QtWindow() override;

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

        /** Destroys the renderer's window. 
         */
        void close() override {
            closing_ = true;
            RendererWindow::close();
            QWidget::close();
        }

        void schedule(std::function<void()> event, Widget * widget) override {
            Super::schedule(event, widget);
            emit QtApplication::Instance()->tppUserEvent();
        }

        using RendererWindow<QtWindow, QWidget*>::schedule; 

    signals:
       void tppRequestUpdate();
       void tppShowFullScreen();
       void tppShowNormal();

    protected:

        /** Renders the window. 
         
            Instead of renderring immediately the method simply emits the update() event, which will in turn call the paintEvent() method which does the actual rendering on Qt. 
         */
        void render(Rect const & rect) override {
            MARK_AS_UNUSED(rect);
            emit tppRequestUpdate();
        }

        void requestClipboard(Widget * sender) override;

        void requestSelection(Widget * sender) override;

        void setClipboard(std::string const & contents) override;

        void setSelection(std::string const & contents, Widget * owner) override;

        void clearSelection(Widget * owner) override;

    private:
        friend class QtApplication;
        friend class RendererWindow<QtWindow, QWidget*>;
        typedef RendererWindow<QtWindow, QWidget *> Super;

        /** Creates the renderer window of appropriate size using the default font and zoom of 1.0. 
         */
        QtWindow(std::string const & title, int cols, int rows, EventQueue & eventQueue);

        /** Qt's resize event.
         */
        void resizeEvent(QResizeEvent* ev) override {
            QWidget::resizeEvent(ev);
            windowResized(ev->size().width(), ev->size().height());
        }

        void closeEvent(QCloseEvent * ev) override {
            if (closing_) {
                QWidget::closeEvent(ev);
            } else {
                ev->ignore();
                // we have to schedule the event since the request close will generate its own call to close() and therefore closingEvent
                schedule([this](){
                    requestClose();
                });
            }
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

        void enterEvent(QEvent * ev) override;

        void leaveEvent(QEvent * ev) override;

        //@}


        /** \name Rendering Functions
         */
        //@{
        /** Qt's paint event just delegates to the render method of the RendererWindow. 
         */
        void paintEvent(QPaintEvent * ev) override {
            MARK_AS_UNUSED(ev);
            Super::render(Rect{Super::size()});
        }
        void initializeDraw() {
            painter_.begin(this);
        }

        void finalizeDraw() {
            changeBackgroundColor(backgroundColor());
            if (sizePx_.width() % cellSize_.width() != 0)
                painter_.fillRect(QRect{Super::width() * cellSize_.width(), 0, sizePx_.width() % cellSize_.width(), sizePx_.height()}, painter_.brush());
            if (sizePx_.height() % cellSize_.height() != 0)
                painter_.fillRect(QRect{0, Super::height() * cellSize_.height(), sizePx_.width(), sizePx_.height() % cellSize_.height()}, painter_.brush());
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
                painter_.fillRect(col * cellSize_.width(), (row + 1 - fontHeight)  * cellSize_.height(), cellSize_.width() * fontWidth, cellSize_.height() * fontHeight, painter_.brush());   
            }
            if ((cp != 32) && (!state_.font().blink() || BlinkVisible())) {
                painter_.drawText(col * cellSize_.width(), (row + 1 - state_.font().height()) * cellSize_.height() + font_->ascent(), QString::fromUcs4(&cp, 1));
            }
            ++glyphRunSize_;
        }

        /** Updates the current font.
         */
        void changeFont(ui::Font font) {
            font_ = QtFont::Get(font, cellSize_);
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
            if (!state_.font().blink() || BlinkVisible()) {
                if (state_.font().underline())
                    painter_.fillRect(glyphRunStart_.x() * cellSize_.width(), glyphRunStart_.y() * cellSize_.height() + font_->underlineOffset(), cellSize_.width() * glyphRunSize_, font_->underlineThickness(), decorationBrush_);
                if (state_.font().strikethrough())
                    painter_.fillRect(glyphRunStart_.x() * cellSize_.width(), glyphRunStart_.y() * cellSize_.height() + font_->strikethroughOffset(), cellSize_.width() * glyphRunSize_, font_->strikethroughThickness(), decorationBrush_);
            }
        }

        void drawBorder(int col, int row, Border const & border, int widthThin, int widthThick) {
            int left = col * cellSize_.width();
            int top = row * cellSize_.height();
            int widthTop = border.top() == Border::Kind::None ? 0 : (border.top() == Border::Kind::Thick ? widthThick : widthThin);
            int widthLeft = border.left() == Border::Kind::None ? 0 : (border.left() == Border::Kind::Thick ? widthThick : widthThin);
            int widthBottom = border.bottom() == Border::Kind::None ? 0 : (border.bottom() == Border::Kind::Thick ? widthThick : widthThin);
            int widthRight = border.right() == Border::Kind::None ? 0 : (border.right() == Border::Kind::Thick ? widthThick : widthThin);

            if (widthTop != 0)
                painter_.fillRect(left, top, cellSize_.width(), widthTop, painter_.brush());
            if (widthBottom != 0)
                painter_.fillRect(left, top + cellSize_.height() - widthBottom, cellSize_.width(), widthBottom, painter_.brush());
            if (widthLeft != 0) 
                painter_.fillRect(left, top + widthTop, widthLeft, cellSize_.height() - widthTop - widthBottom, painter_.brush());
            if (widthRight != 0) 
                painter_.fillRect(left + cellSize_.width() - widthRight, top + widthTop, widthRight, cellSize_.height() - widthTop - widthBottom, painter_.brush());
        }
        //@}

        static ui::Key GetStateModifiers(Qt::KeyboardModifiers const & modifiers);
        static ui::Key GetKey(int qtKey, ui::Key modifiers, bool pressed);

        QPainter painter_;
        QtFont * font_;

        QBrush decorationBrush_;
        QBrush borderBrush_;

        Point glyphRunStart_;
        int glyphRunSize_;

        /** True if the window is in the process of closing itself, i.e. the closeEvent should be accepted unconditionally. */
        bool closing_ = false;

    }; // tpp::QtWindow

} // namespace tpp

#endif

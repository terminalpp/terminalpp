#pragma once

#include "../stamp.h"
#include "helpers/stamp.h"

#include "ui/widget.h"
#include "ui/builders.h"

namespace tpp {

    /** A simple about box.

        Displays the terminal version and basic information. The about box dims the screen and then displays a centered about box with fixed width and height (60x10 cells).

        TODO this would be better served by a modal show of a widget perhaps? 
     */
    class AboutBox : public ui::Widget {
    public:
        AboutBox():
            Widget(),
            lastKey_(ui::Key::Invalid) {
            using namespace ui;
        }

        void show() {
            setVisible(true);
            setFocused(true);
        }

        ui::Event<void> onDismissed;

        using Widget::setVisible;

    protected:

        void updateFocused(bool value) override {
            Widget::updateFocused(value);
            if (visible() && value == false) {
                setVisible(false);
                onDismissed(this);
            }
        }

        void mouseClick(int col, int row, ui::MouseButton button, ui::Key modifiers) override {
            MARK_AS_UNUSED(col);
            MARK_AS_UNUSED(row);
            MARK_AS_UNUSED(button);
            MARK_AS_UNUSED(modifiers);
            if (lastKey_ == ui::Key::Invalid) {
                setVisible(false);
                onDismissed(this);
            }
        }

        void keyDown(ui::Key k) override {
            if (lastKey_ == ui::Key::Invalid)
                lastKey_ = k.code();
        }

        void keyUp(ui::Key k) override {
            if (k.code() == lastKey_) {
                lastKey_ = ui::Key::Invalid;
                setVisible(false);
                onDismissed(this);
            }
        }

        void paint(ui::Canvas & canvas) override {
            using namespace ui;
            canvas.fill(Rect::FromWH(canvas.width(), canvas.height()), Brush(Color::Black.withAlpha(128)));
            int x = (canvas.width() - 60) / 2;
            int y = (canvas.height() - 10) / 2;
            canvas.fill(Rect::FromCorners(x, y, x + 60, y + 10), Brush(Color::Blue));
            canvas.textOut(Point(x + 20,y + 1), "Terminal++", Color::White, ui::Font().setSize(2));
            helpers::Stamp stamp = helpers::Stamp::Stored();
            if (stamp.version().empty()) {
                canvas.textOut(Point(x + 5, y + 3), STR("commit:   " << stamp.commit() << (stamp.clean() ? "" : "*")), Color::White, ui::Font());
                canvas.textOut(Point(x + 15, y + 4), stamp.time(), Color::White, ui::Font());
            } else {
                canvas.textOut(Point(x + 5, y + 3), STR("version:  " << stamp.version()), Color::White, ui::Font());
                canvas.textOut(Point(x + 15, y + 4), STR(stamp.commit() << (stamp.clean() ? "" : "*")), Color::White, ui::Font());
                canvas.textOut(Point(x + 15, y + 5), stamp.time(), Color::White, ui::Font());
            }
            canvas.textOut(Point(x + 5, y + 7), STR("platform: " << ARCH << " " << ARCH_SIZE << " " << ARCH_COMPILER << " " << ARCH_COMPILER_VERSION << " " << stamp.buildType()), Color::White, ui::Font());
            canvas.textOut(Point(x + 20, y + 9), "Hit a key to dismiss", Color::White, ui::Font());
            // finally, draw the border
            canvas.borderRect(Rect::FromCorners(x, y, x + 60, y + 10), ui::Color::White, false);
        }

    private:
        unsigned lastKey_;
    }; // tpp::AboutBox

} // namespace tpp
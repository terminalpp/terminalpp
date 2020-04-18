#pragma once

#include "../container.h"

namespace ui {

    class ModalPane : public Container {
    public:

        ModalPane():
            background_{Color::Black.withAlpha(128)} {
        }

        Color dimColor() const {
            return background_;
        }

        virtual void setDimColor(Color value) {
            if (background_ != value) {
                background_ = value;
                /*
                if (background_.opaque())
                    setOverlay(Overlay::Force);
                else
                    setOverlay(Overlay::Yes);
                */
                repaint();
            }
        }

    protected:

        void paint(Canvas & canvas) override {
            if (background_.opaque()) {
                canvas.setBg(background_);
                canvas.fillRect(canvas.rect());
            } else {
                canvas.fillRect(canvas.rect(), background_);
            }
            Container::paint(canvas);
        }

        Color background_;

    };


} // namespace ui
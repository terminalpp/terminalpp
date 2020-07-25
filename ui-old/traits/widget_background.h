#pragma once

#include "../geometry.h"
#include "../canvas.h"

#include "trait_base.h"

namespace ui {

    template<typename T>
    class WidgetBackground : public TraitBase<WidgetBackground, T> {
    public:

        Color const & background() const {
            return background_;
        }

        virtual void setBackground(Color const & value) {
            if (background_ != value) {
                background_ = value;
                downcastThis()->repaint();
            }
        }

    protected:
        using TraitBase<WidgetBackground, T>::downcastThis;

        WidgetBackground(Color background = Color::Black):
            background_{background} {
        }

        virtual Widget * propagatePaintTarget(Widget * sender, Widget * target) {
            if (sender == downcastThis() && ! background_.opaque() && downcastThis()->parent() != nullptr)
                return downcastThis()->parent();
            else
                return target;
        }

        virtual void paint(Canvas & canvas) {
            if (background_.opaque()) {
                canvas.setBg(background_);
                canvas.fillRect(canvas.rect());
            } else {
                canvas.fillRect(canvas.rect(), background_);
            }
        }

        Color background_;

    }; // ui::WidgetBackground


} // namespace ui
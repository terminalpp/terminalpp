#pragma once

#include "../geometry.h"
#include "../canvas.h"

#include "trait_base.h"

namespace ui {

    template<typename T>
    class WidgetBackround : public TraitBase<Box, T> {
    public:

        Color const & background() const {
            return background_;
        }

        virtual void setBackground(Color const & value) {
            if (background_ != value) {
                background_ = value;
                if (! border_.empty() || background_.opaque())
                    setWidgetOverlay(Widget::Overlay::Force);
                else 
                    setWidgetOverlay(Widget::Overlay::Yes);
                downcastThis()->repaint();
            }
        }

    protected:
        using TraitBase<WidgetBackground, T>::downcastThis;
        using TraitBase<WidgetBackground, T>::widgetOverlay;
        using TraitBase<WidgetBackground, T>::setWidgetOverlay;

        void paint(Canvas & canvas) {
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
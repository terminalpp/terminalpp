#pragma once

#include "../geometry.h"
#include "../canvas.h"

#include "trait_base.h"

namespace ui {

    template<typename T>
    class WidgetBorder : public TraitBase<WidgetBorder, T> {
    public:

        Border const & border() const {
            return border_;
        }

        virtual void setBorder(Border const & value) {
            if (border_ != value) {
                border_ = value;
                downcastThis()->repaint();
            }
        }

    protected:
        using TraitBase<WidgetBorder, T>::downcastThis;

        /** TODO Only requires the child repaint if its rectangle intersects with the border. 
         */
        virtual bool requireChildToDelegatePaint(Widget * child) {
            MARK_AS_UNUSED(child);
            return ! border_.empty();
        }

        void paint(Canvas & canvas) {
            if (! border_.empty())
                canvas.addFinalizer([this](Canvas & canvas) {
                    canvas.drawBorderRect(border_, canvas.rect());
                });
        }

        Border border_;

    }; // ui::WidgetBorder

} // namespace ui
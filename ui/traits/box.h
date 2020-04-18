#pragma once

#include "../geometry.h"
#include "../canvas.h"

#include "trait_base.h"

namespace ui {

    /** Box-like element drawing. 
     
        Provides the background brush and border specification for boxed ui elements. 
     */
    template<typename T>
    class Box : public TraitBase<Box, T> {
    public:

        Border const & border() const {
            return border_;
        }

        virtual void setBorder(Border const & value) {
            if (border_ != value) {
                border_ = value;
                /*
                if (! border_.empty() || background_.opaque())
                    setWidgetOverlay(Widget::Overlay::Force);
                else 
                    setWidgetOverlay(Widget::Overlay::Yes);
                */
                downcastThis()->repaint();
            }
        }

    protected:
        using TraitBase<Box, T>::downcastThis;
        //using TraitBase<Box, T>::setWidgetOverlay;

        Box():
            background_(Color::Black) {
        }

        explicit Box(Color color):
            background_{color} {
        }

        Box(Brush const & background, Border const & border):
            background_{background},
            border_{border} {
        }

        virtual bool delegatePaintToParent() {
            return ! background_.opaque();
        }

        virtual bool requireChildrenToDelegatePaint() {
            return ! border_.empty();
        }


        void paint(Canvas & canvas) {
            if (background_.opaque()) {
                canvas.setBg(background_);
                canvas.fillRect(canvas.rect());
            } else {
                canvas.fillRect(canvas.rect(), background_);
            }
            if (! border_.empty())
                canvas.addFinalizer([this](Canvas & canvas) {
                    canvas.drawBorderRect(border_, canvas.rect());
                });
        }

        Color background_;
        Border border_;

    }; // ui::Box

    /** Box drawing in different control states.
     */

    template<typename T>
    class DynamicBox : public Box<T> {
    public:

    }; // ui::DynamicBox

} // namespace ui
#pragma once


#include "../shapes.h"
#include "../canvas.h"

#include "trait_base.h"


namespace ui {

    /** Box-like element drawing. 
     
        Provides the background brush and border specification for boxed ui elements. 
     */
    template<typename T>
    class Box : public TraitBase<Box, T> {
    public:
        Brush const & background() const {
            return background_;
        }

        void setBackground(Brush const & value) {
            if (background_ != value)
                updateBackground(value);
        }

        Border const & border() const {
            return border_;
        }

        void setBorder(Border const & value) {
            if (border_ != value)
                updateBorder(value);
        }

    protected:
        using TraitBase<Box, T>::downcastThis;
        using TraitBase<Box, T>::downcastSetForceOverlay;

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

        virtual void updateBackground(Brush const & value) {
            background_ = value;
            downcastSetForceOverlay(!background_.color.opaque());
            downcastThis()->repaint();
        }

        virtual void updateBorder(Border const & value) {
            border_ = value;
            downcastThis()->repaint();
        }

        void paint(Canvas & canvas) {
            canvas.fill(canvas.rect(), background_);
            if (border_.visible())
                canvas.borderRect(canvas.rect(), border_);
        }

        Brush background_;
        Border border_;

    }; // ui::Box

    /** Box drawing in different control states.
     */

    template<typename T>
    class DynamicBox : public Box<T> {
    public:

    }; // ui::DynamicBox

} // namespace ui
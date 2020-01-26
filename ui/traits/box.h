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

    protected:
        using TraitBase<Box, T>::downcastThis;
        using TraitBase<Box, T>::downcastSetForceOverlay;

        Box():
          background_(Color::Black) {
        }

        explicit Box(Color color):
          background_(color) {
        }

        virtual void updateBackground(Brush const & value) {
            background_ = value;
            downcastSetForceOverlay(!background_.color.opaque());
            downcastThis()->repaint();
        }

        void paint(Canvas & canvas) {
            canvas.fill(canvas.rect(), background_);
        }

        Brush background_;

    }; // ui::Box

} // namespace ui
#pragma once

#include "../widget.h"

namespace ui {

    template<template<typename> typename X, typename T>
    class TraitBase {
    protected:

        T * downcastThis() {
            return static_cast<T*>(this);
        }

        T const * downcastThis() const {
            return static_cast<T const *>(this);
        }

        /*
        void setWidgetOverlay(bool ) {
            downcastThis()->setOverlay(overlay);
        }
        */

    }; // ui::TraitBase

} // namespace ui
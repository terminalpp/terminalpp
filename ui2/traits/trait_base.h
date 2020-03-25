#pragma once

namespace ui2 {

    template<template<typename> typename X, typename T>
    class TraitBase {
    protected:

        T * downcastThis() {
            return static_cast<T*>(this);
        }

        T const * downcastThis() const {
            return static_cast<T const *>(this);
        }

        void downcastSetForceOverlay(bool value) {
            downcastThis()->setForceOverlay(value);
        }

    }; // ui::TraitBase

} // namespace ui
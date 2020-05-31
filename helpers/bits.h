#pragma once

#include "helpers.h"

HELPERS_NAMESPACE_BEGIN

    /** Sets or clears the given mask. 
     */
    template<typename T>
    T SetBit(T const & value, T const & mask, bool enable) {
        if (enable)
            return value | mask;
        else
            return value & ~ mask;
    }

    /** Sets bits in the mask to the given value leaving the rest intact. 
     */ 
    template<typename T>
    T SetBits(T const & value, T const & mask, T const & bits) {
        return (value & ~ mask) | bits;
    } 


HELPERS_NAMESPACE_END
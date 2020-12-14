#include "helpers.h"

HELPERS_NAMESPACE_BEGIN

    /** Similar to memcpy, but actually works on objects using their assignment operator. 
     
        This makes the function an almost drop-in replacement for memcpy if the data copied are not PODs.
     */
    template<typename T>
    void MemCopy(T * target, T const * source, size_t size) {
        for (size_t i = 0; i < size; ++i)
            target[i] = source[i];
    }

HELPERS_NAMESPACE_END
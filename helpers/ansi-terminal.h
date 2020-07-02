#pragma once

#include "helpers.h"

HELPERS_NAMESPACE_BEGIN

/** Stream manipulators for selected ANSI escape sequences. 
 */

namespace ansi {

    char const * constexpr ESC = "\033";
    char const * constexpr CSI = "\033[";

    /** Sets the cursor position to given coordinates. 
     
        Unlike the ansi sequences, the indices start from 0. 
     */
    struct SetCursor {
        SetCursor(int x, int y):
            x{x},
            y{y} {
        }

    private:
        int x;
        int y;

        friend std::ostream & operator << (std::ostream & s, SetCursor const & x) {
            s << CSI << (x.y + 1) << ';' << (x.x + 1) << 'H';
            return s;
        }        
    };


} // namespace ansi

HELPERS_NAMESPACE_END
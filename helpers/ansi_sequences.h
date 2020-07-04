#pragma once

#include "helpers.h"

HELPERS_NAMESPACE_BEGIN

/** Stream manipulators for selected ANSI escape sequences. 
 */

namespace ansi {

    constexpr char const * const ESC = "\033";
    constexpr char const * const CSI = "\033[";

    struct AlternateMode {
        AlternateMode(bool value = true): value{value} { };
    private:
        bool const value;
        friend std::ostream & operator << (std::ostream & s, AlternateMode const & x) {
            s << CSI << "?1049" << (x.value ? 'h' : 'l');
            return s;
        }        
    };

    /** Sets the cursor position to given coordinates. 
     
        Unlike the ansi sequences, the indices start from 0. 
     */
    struct SetCursor {
        SetCursor(int x, int y): x{x}, y{y} { }
    private:
        int const x, y;
        friend std::ostream & operator << (std::ostream & s, SetCursor const & x) {
            s << CSI << (x.y + 1) << ';' << (x.x + 1) << 'H';
            return s;
        }        
    };

    struct SGRReset {
        SGRReset() { }
    private:
        friend std::ostream & operator << (std::ostream & s, SGRReset const & x) {
            MARK_AS_UNUSED(x);
            s << CSI << "0m";
            return s;
        }
    };


    struct Fg {
        Fg(unsigned char r, unsigned char g, unsigned char b): r{r}, g{g}, b{b} { }
    private:
        unsigned char const r, g, b;
        friend std::ostream & operator << (std::ostream & s, Fg const & x) {
            s << CSI << "38;2;" << x.r << ';' << x.g << ';' << x.b << 'm';
            return s;
        }
    };

    struct Bg {
        Bg(unsigned char r, unsigned char g, unsigned char b): r{r}, g{g}, b{b} { }
    private:
        unsigned char const r, g, b;
        friend std::ostream & operator << (std::ostream & s, Bg const & x) {
            s << CSI << "48;2;" << x.r << ';' << x.g << ';' << x.b << 'm';
            return s;
        }
    };

    struct Bold {
        Bold(bool value = true): value{value} { }
    private:
        bool const value;
        friend std::ostream & operator << (std::ostream & s, Bold const & x) {
            s << CSI << (x.value ? "1m" : "22m");
            return s;
        }
    };

    struct Italic {
        Italic(bool value = true): value{value} { }
    private:
        bool const value;
        friend std::ostream & operator << (std::ostream & s, Italic const & x) {
            s << CSI << (x.value ? "3m" : "23m");
            return s;
        }
    };

    struct Underline {
        Underline(bool value = true): value{value} { }
    private:
        bool const value;
        friend std::ostream & operator << (std::ostream & s, Underline const & x) {
            s << CSI << (x.value ? "4m" : "24m");
            return s;
        }
    };

    struct Strikethrough {
        Strikethrough(bool value = true): value{value} { }
    private:
        bool const value;
        friend std::ostream & operator << (std::ostream & s, Strikethrough const & x) {
            s << CSI << (x.value ? "9m" : "29m");
            return s;
        }
    };

    struct Blink {
        Blink(bool value = true): value{value} { }
    private:
        bool const value;
        friend std::ostream & operator << (std::ostream & s, Blink const & x) {
            s << CSI << (x.value ? "5m" : "25m");
            return s;
        }
    };




} // namespace ansi

HELPERS_NAMESPACE_END
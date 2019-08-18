# V0.2

- add the extra attributes drawing
- terminal should have an event to be called when a line is to be scrolled out

- cell builders should use && as well so that we can use them on temporary objects
- resizing the terminal does sometimes eats stuff

## Bugs and missing features

- add zoom out? and default zoom? 

### Win32 Bugs

### X11 Bugs

# Code Fixes

# Code Improvements 

- improve logging - the logging overhead is gigantic, find a way to lower it
- add tests
- add size selection to PTY? or not really, since it can always be resized. 

# Long Term Goals

- non-english double width characters don't work properly (at all)
- vt100 should have palette access ready
- xim is wrong, it works on linux, but fails on mac
- then at some point in the future, helpers, tpp and ui+vterm should go in separate repos
- start documentation - see https://devblogs.microsoft.com/cppblog/clear-functional-c-documentation-with-sphinx-breathe-doxygen-cmake/

# UI

- onChange handler for generic changes
- make canvas do more
- mouseFocus, should it disapper when mouse moves from parent to child? 
- determine how keyboard focus is obtained automatically
- add extra styles to the font (left-top-bottom-right) borders (??)
- how to deal with palette? (like a global palette object, and being able to set palette mappings for the widgets? 

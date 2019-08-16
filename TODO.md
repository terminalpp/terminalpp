# V0.2

- determine the closing mechanism (ie. session v2)
- add the extra attributes drawing
- terminal should have an event to be called when a line is to be scrolled out

- cell builders should use && as well so that we can use them on temporary objects
- resizing the terminal does sometimes eats stuff

- terminal's on title change should propagate to the root window's title change 

## Bugs and missing features

- check asserts, some should actually be OSCHECKS
- add zoom out? and default zoom? 

### Win32 Bugs

- does not print version (I guess uninitialized console)

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

- add extra styles to the font (left-top-bottom-right) borders (??)
- how to deal with palette? (like a global palette object, and being able to set palette mappings for the widgets? 

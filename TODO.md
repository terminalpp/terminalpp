
- fonts calculate their offsets and double width, but rendering does not support the offsets



## Bugs and missing features

- add zoom out? and default zoom? 
- add the extra attributes drawing
- cell builders should use && as well so that we can use them on temporary objects
- resizing the terminal does sometimes eats stuff
- add transparency?

### Win32 Bugs

### X11 Bugs

# Code Fixes

# Code Improvements 

- improve logging - the logging overhead is gigantic, find a way to lower it
- add JSON schema and validator support
- add tests
- add size selection to PTY? or not really, since it can always be resized. 

# Long Term Goals

- non-english double width characters don't work properly (at all)
- xim is wrong, it works on linux, but fails on mac
- then at some point in the future, helpers, tpp and ui+vterm should go in separate repos
- start documentation - see https://devblogs.microsoft.com/cppblog/clear-functional-c-documentation-with-sphinx-breathe-doxygen-cmake/

# UI

- onChange handler for generic changes
- make canvas do more
- mouseFocus, should it disapper when mouse moves from parent to child? 
- determine how keyboard focus is obtained automatically
- how to deal with palette? (like a global palette object, and being able to set palette mappings for the widgets? 

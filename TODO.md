## Bugs and missing features

- check asserts, some should actually be OSCHECKS
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

# UI

- hide properties not meant to be public immediately
- horizontal and vertical layouts
- size hints in controls
- scrolling (make clientWidth and height virtual functions and scrollable controls allow larger client width and height than the actual area? 
- onChange handler for generic changes
- add events
- make canvas do more

- mouseFocus, should it disapper when mouse moves from parent to child? 



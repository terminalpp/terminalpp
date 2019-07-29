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
- then at some point in the future, helpers, tpp and ui+vterm should go in separate repos
- 
# UI

- hide properties not meant to be public immediately
- onChange handler for generic changes
- add events
- make canvas do more
- terminal widget
- mouseFocus, should it disapper when mouse moves from parent to child? 

- add extra styles to the font (left-top-bottom-right) borders (??)
- add color to the font decorations 
- how to deal with palette? (like a global palette object, and being able to set palette mappings for the widgets? 

- transparent controls must repaint their parent as well when they repaint themselves, this can easily mean that when a control is transparent, its overlay is always true :)


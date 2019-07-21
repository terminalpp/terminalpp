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
- add comments to bypass

# Long Term Goals

- non-english double width characters don't work properly (at all)
- vt100 should have palette access ready
- xim is wrong, it works on linux, but fails on mac

# UI

- horizontal and vertical layouts
- size hints in controls
- client area in container, i.e. a possibility of a border around the container
- add events
- propagate events through windows
- hide properties not meant to be public immediately
- make canvas do more

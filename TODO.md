# TODOs

- look at all TODOs and fix as many as possible, or convert to issues

## Bugs and missing features

- configuration entirely via macros in config.h
- selection + middle button is paste (via primary selection in X11)
- screen buffer not properly preserved over resized on empty lines (like time of a command)

### Win32 Bugs

### X11 Bugs

- fullscreen does not work
- the rendering speed is not the best. It works for work just fine though. Althoiugh on WSL + vcxsrv the speed is actually quite good

# Code Fixes

# Code Improvements 

- screen should have scrolling functions in itself (?) - I guess yes...
- improve logging - the logging overhead is gigantic, find a way to lower it
- add tests & CI
- add size selection to PTY? or not really, since it can always be resized. 
- add comments to bypass

# Long Term Goals

- non-english double width characters don't work properly (at all)
- vt100 should have palette access ready

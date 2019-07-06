# TODOs

- get rid of warnings on windows and linux
- look at all TODOs and fix as many as possible, or convert to issues

## Bugs and missing features

### Win32 Bugs

- verify that middle button selection works

### X11 Bugs

- selection fg color is wrong - seems to depend on blinking (can be in Windows too)

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

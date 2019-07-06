# TODOs

- get rid of warnings on windows and linux
- look at all TODOs and fix as many as possible, or convert to issues

## Bugs and missing features

- rewrite buffer resize into the new by row cell arrangement
- add Cell constructor and remove vt100 delete and insert lines, call screen directly
- add command line arguments so that terminal can be proper terminal emulator replacement in Linux
- add zoom out? and default zoom? 

### Win32 Bugs

- create console window conditionally on NDEBUG flag

### X11 Bugs

# Code Fixes

# Code Improvements 

- improve logging - the logging overhead is gigantic, find a way to lower it
- add tests & CI
- add size selection to PTY? or not really, since it can always be resized. 
- add comments to bypass

# Long Term Goals

- non-english double width characters don't work properly (at all)
- vt100 should have palette access ready

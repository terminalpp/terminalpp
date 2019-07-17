## Bugs and missing features

- VT100 should be fault tolerant and not assert everything, just do nothing upon failure in release
- not just VT, all asserts should be checked and become OSerrors instead
- add zoom out? and default zoom? 

### Win32 Bugs

### X11 Bugs

- xim is wrong, it works on linux, but fails on mac

# Code Fixes

# Code Improvements 

- improve logging - the logging overhead is gigantic, find a way to lower it
- add tests
- add size selection to PTY? or not really, since it can always be resized. 
- add comments to bypass
- make bypass record the stream for more realistic benchmark (but maybe this is useless)

# Long Term Goals

- non-english double width characters don't work properly (at all)
- vt100 should have palette access ready

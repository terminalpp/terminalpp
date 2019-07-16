## Bugs and missing features

- when windows loses focus, the key state may change, perhaps the modifiers key state should be added to mouse events as well and that way vt100 does not have to keep the state in itself, but instead the app will? 
- or it must be notified by the app about input state change

- VT100 should be fault tolerant and not assert everything, just do nothing upon failure in release
- not just VT, all asserts should be checked and become OSerrors instead
- add command line arguments so that terminal can be proper terminal emulator replacement in Linux
- add zoom out? and default zoom? 
- args should by default parse version and help requests, version should be stamped from the git repo, I guess by creating simple c program that generates the header file appropriately
- platform macros should be set in helpers

### Win32 Bugs

### X11 Bugs

# Code Fixes

# Code Improvements 

- improve logging - the logging overhead is gigantic, find a way to lower it
- add tests
- add size selection to PTY? or not really, since it can always be resized. 
- add comments to bypass
- maek bypass record the stream for more realistic benchmark (but maybe this is useless)

# Long Term Goals

- non-english double width characters don't work properly (at all)
- vt100 should have palette access ready

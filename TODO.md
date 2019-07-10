## Bugs and missing features

- VT100 should be fault tolerant and not assert everything, just do nothing upon failure in release
- add command line arguments so that terminal can be proper terminal emulator replacement in Linux
- add zoom out? and default zoom? 

### Win32 Bugs

- fix the resources builder to read the icon from images, not from the tpp source file
- move the resource to directwrite folder

### X11 Bugs

- app icon, kinda works, clean the code

# Code Fixes

# Code Improvements 

- improve logging - the logging overhead is gigantic, find a way to lower it
- add tests & CI
- add size selection to PTY? or not really, since it can always be resized. 
- add comments to bypass
- maek bypass record the stream for more realistic benchmark (but maybe this is useless)

# Long Term Goals

- non-english double width characters don't work properly (at all)
- vt100 should have palette access ready

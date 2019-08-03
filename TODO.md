# V0.2

- add VT100 and goodies
- then add localpty as well for Windows
- add events for the terminal (notification, etc.)

- make terminal an UI widget
- make Font contain stuff from nativeHandle on DirectWrite (like underline & strikethrough positions & thickness)
- move the renderer in window.h to use glyph runs in all instances

- add focus in/out events to root window
- determine the closing mechanism (ie. session v2)
- of course, add X11 - and localpTY on linux...
- consider keeping track of what needs to be changed
- add the extra attributes drawing
- terminal should have an event to be called when a line is to be scrolled out


- add the FPS timer to the terminal
- and add cursor display to the ui root window and figure out how the two should be reconciled


## Bugs and missing features

- check asserts, some should actually be OSCHECKS
- add zoom out? and default zoom? 

### Win32 Bugs

- get rid of the sound when going fullscreen -> non-fullscreen

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

- onChange handler for generic changes
- make canvas do more
- terminal widget
- keyboard focus and events
- mouseFocus, should it disapper when mouse moves from parent to child? 

- add extra styles to the font (left-top-bottom-right) borders (??)
- add color to the font decorations 
- how to deal with palette? (like a global palette object, and being able to set palette mappings for the widgets? 

# Terminal Simplification

Terminal can be simplified a lot by saying that the UI controls the terminal and that the ui::Terminal is its renderer. 

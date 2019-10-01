- borders should be thin, thick, full
- selection should be cancelled when window gets defocused
- maybe return widget from other mouse things as well (not just mouse down and up)

- clean shapes - rect, to provide the points for corners on demand

- how to deal with scrollbars - i.e. who and when and how draws them? (and reacts to them visually) - perhaps have them as extra widgets (which is a bit wasteful if they keep the same info as the scrollable object, or maybe the scrollable object can point to them?)

- how to deal with repaint requests - there should be an extra method that requests a repaint when appropriate, but the widget itself can determine when to do it, as opposed to repaint which repaints immediately - maybe the immediate repaint should not be force-able if overriden in the widget? 

- selection does not work with history at the moment

- history sucks - only shows text, other information should be captured too

- when terminal is resized the stuff that does not fit should go to history as well - this may be hard if we resize in the presence of double buffers - maybe terminal's on line out always adds the line, but VT100 decides to only call it when not in alternate mode... 
- scrolling when resized asserts... 

## Bugs and missing features

- add zoom out? and default zoom? 
- add the extra attributes drawing
- cell builders should use && as well so that we can use them on temporary objects
- resizing the terminal does sometimes eats stuff
- ssh plain does not display boxes in mc well

### Win32 Bugs

### X11 Bugs

- transparency of the background does not seem to work

# Code Fixes

# Code Improvements 

- improve logging - the logging overhead is gigantic, find a way to lower it
- add JSON schema and validator support
- add tests
- add size selection to PTY? or not really, since it can always be resized. 

# Long Term Goals

- xim is wrong, it works on linux, but fails on mac
- then at some point in the future, helpers, tpp and ui+vterm should go in separate repos
- start documentation - see https://devblogs.microsoft.com/cppblog/clear-functional-c-documentation-with-sphinx-breathe-doxygen-cmake/

# UI

- add showModal(Widget *) to root window, which would dim the background and then display the widget centered
- allow container to reverse order of its elements (so that maximized and horizontal layouts can be used nicely with header lines both up and down)
- onChange handler for generic changes
- make canvas do more
- mouseFocus, should it disapper when mouse moves from parent to child? 
- how to deal with palette? (like a global palette object, and being able to set palette mappings for the widgets? 

sudo apt install doxygen python3-pip
pip3 install sphinx breathe sphinx_rtd_theme

# Add Transparency?

On Windows:

https://msdn.microsoft.com/en-us/magazine/ee819134.aspx?f=255&MSPPError=-2147217396
and use layered windows

sudo snap install --classic snapcraft
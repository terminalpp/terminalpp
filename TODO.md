- clean shapes - rect, to provide the points for corners on demand
- on linux, dismiss of the about box does not work
- on both - double size characters extend blue background to places where it should not have happened (perhaps the draw rect should also set font size to normal?)

## Bugs and missing features

- add zoom out? and default zoom? 
- add the extra attributes drawing
- cell builders should use && as well so that we can use them on temporary objects
- resizing the terminal does sometimes eats stuff
- ssh plain does not display boxes in mc well

### Win32 Bugs

### X11 Bugs

- message boxes are not drawn
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

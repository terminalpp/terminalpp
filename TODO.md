## Bugs and missing features

- add zoom out? and default zoom? 
- add the extra attributes drawing
- cell builders should use && as well so that we can use them on temporary objects
- resizing the terminal does sometimes eats stuff
- ssh plain does not display boxes in mc well
- settings.json versioning

### Win32 Bugs

### X11 Bugs

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

- onChange handler for generic changes
- make canvas do more
- mouseFocus, should it disapper when mouse moves from parent to child? 
- determine how keyboard focus is obtained automatically
- how to deal with palette? (like a global palette object, and being able to set palette mappings for the widgets? 

sudo apt install doxygen python3-pip
pip3 install sphinx breathe sphinx_rtd_theme

# Add Transparency?

On Windows:

https://msdn.microsoft.com/en-us/magazine/ee819134.aspx?f=255&MSPPError=-2147217396
and use layered windows

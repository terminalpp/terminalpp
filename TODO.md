## Different font sizes

- Rendering does not support multiple sizes
- add default settings doubleWidth font check
- doublewidth font centering does not work for all glyphs if not monospaced (icons)

## Font Fallback

- do font fallback & different sizes first on Windows completely, then look at linux
- the fallback font dimensions is not changed yet
- where to store the cache of the font fallbacks? Perhaps on the "native handle" which is not really a native handle any more so should be renamed
- font fallback does not work for emoticons

## Bugs and missing features

- add zoom out? and default zoom? 
- add the extra attributes drawing
- cell builders should use && as well so that we can use them on temporary objects
- resizing the terminal does sometimes eats stuff
- add transparency?
- ssh plain does not display boxes in mc well

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


# Roadmap

The following is very short description of the versions planned and their main new features. There are no set release dates, a release will happen once the planned features are implemented and tested a bit.

### v0.5

- borders should be thin, thick, full
- support for opening remote files by extra escape sequences (basic)
- ideally snap store will be vetted by this time
- Windows Store version as well

### v0.6

- multiple sessions

### v0.7

- systray support

### v1.0

- polish and bugfixing of stuff, some beta releases and "testing"

### Later

- keyboard shortcuts, actions, better settings than JSON, etc. 

# Changelog

### v0.4

- support terminal history, scrolling, selection from it, etc. 
- will open settings.json for editing when requested





## Old stuffs

- should client & scroll rects & stuff be protected by default?
- clean shapes - rect, to provide the points for corners on demand

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
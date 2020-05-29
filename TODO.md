The following is very short description of the versions planned and their main new features. There are no set release dates, a release will happen once the planned features are implemented and tested a bit.

## Next version: v0.7.1

Restructured release process and minor fixes:

- msix package
- Ubuntu 20.04 bypass

### TODO

> These are items I currently work on towards the next version. When they are done, they die.  

Documentation:

- write documentation for layouts and revisit if I want to do anchors (perhaps via a special anchor layout, or via Layout::None that would just notify the child that change has occured) - most likely I do not want this... 
- document size hints 
- add comments to keyboard focus stuff
- mouseUp, Down, WHeel, Move, Click and DoubleClick can propagate to parent, update the documentation
- and so does the keyboard, propagation to parent disabled by default if event enabled 
- document the modal pane & modal trait
- helpers::time timer

Builds & Releases

- ubuntu ppa
- automated MS store submission for the MSIX package
- add benchmarking
- revisit how errors are reported and logged and how things are checked before releases
- add sonarcloud.io

Helpers & Logging & Errors overhaul

- helpers, should they be in helpers namespace?
- helpers filesystem should use paths better
- when a certain exception is raised, it should be logged automatically
- perhaps have the basics all in a single file (log, exceptions, asserts)

Issues to be raised:

- add a way to copy from mouse capturing terminal
- how to & when invalidate selection when there are changes in the terminal's contents
- fillRect should also update border color
- history in alternate mode
- add configuration option to confirm setting clipboard from the terminal

# Roadmap

> This section discusses plan for next versions and drafts the features planned. 

### v0.8

- logging 
- some basic styling for the GUI via a style object, similar to CSS?, more powerful buttons & labels
- color for dimming disabled terminal (via styling)
- scrollview, tooltips, shortcuts and actions and some more UI elements
- tooltip widget can be obtained by renderer and renderer knows its rectangle and can therefore issue repaints as appropriate
- PositionHint: layout / absolute - absolute positioned elements are not updated by layouts
- keyboard focus with tabIndices (this means that the getNextFocusableWidget has to be patched to look for the focusable widgets according to their indices instead of position in the children vector)

- WidgetBorder should only require repaint of children that interfere with the border (can be calculated from their visible rects)
- buffering relayouts (see Layout::setChildOverlay)

- geometry.h is areally bad name for some of the classes defined therein


### v0.9

- even more UI elements
- multiple sessions (in same window)
- when multiple sessions are supported, the RendererWindow can set the font instead of the subclasses

### v0.10

- multiple sessions in multiple windows or tiling
- better bug reporting support and optional checking of updates for non-store installers
- systray support & platform notifications

### v0.11

- proper multiplexing of the terminal sessions (wrt remote files)
- remote files should use absolute path, or some other form of same file detection

### v1.0

- polish and bugfixing of stuff, some beta releases and "testing"
- remove the tpp application from snap (tpp.yaml)

# Unassigned Features

> These are features that I am considering to add, but no immediate plans to do so, noty even sure if they will happen.  

- keyboard shortcuts
- UI actions
- add zoom out? and default zoom? 
- xim is wrong, it works on linux, but fails on mac
- how to deal with palette? (like a global palette object, and being able to set palette mappings for the widgets? 
- add transparency for entire window? on Windows - https://msdn.microsoft.com/en-us/magazine/ee819134.aspx?f=255&MSPPError=-2147217396 and use layered windows



The following is very short description of the versions planned and their main new features. There are no set release dates, a release will happen once the planned features are implemented and tested a bit.

## Next version: v0.7.3 or 0.8

- alternate zoom shortcuts (`C-S--` and `C-S-=`)
- ropen can be interrupted gracefully

- `--session` command-line argument to easily select session

### TODO

> These are items I currently work on towards the next version. When they are done, they die.  

- change settings name to settings.json (GetSettingsFile)

- source package artifact for deb
- release ropen package to snap 
- automatic release to Windows store
- upload to ppa ? 
- update the website ? 

Documentation:

- write documentation for layouts and revisit if I want to do anchors (perhaps via a special anchor layout, or via Layout::None that would just notify the child that change has occured) - most likely I do not want this... 
- document size hints 
- add comments to keyboard focus stuff
- mouseUp, Down, WHeel, Move, Click and DoubleClick can propagate to parent, update the documentation
- and so does the keyboard, propagation to parent disabled by default if event enabled 
- document the modal pane & modal trait
- time timer

Builds & Releases

- add benchmarking
- revisit how errors are reported and logged and how things are checked before releases
- add sonarcloud.io

Helpers & Logging & Errors overhaul

- helpers filesystem should use paths better
- perhaps have the basics all in a single file (log, exceptions, asserts)

Issues to be raised:

- add a way to copy from mouse capturing terminal
- how to & when invalidate selection when there are changes in the terminal's contents
- fillRect should also update border color
- history in alternate mode
- add configuration option to confirm setting clipboard from the terminal

UI version 3

- terminal scrolling disabled for alternate mode

- implement terminal & history resizing
- clear the code
- implement selection & stuff

- contents size is not really updated, but I guess that is ok - there should be sth like contentssize setter which does the update <- CHECK THIS WHAT IT MEANS AND HOW TO DEAL WITH IT
- terminal won't use that and will simply resize its own canvas once it is given it

- cursor
- mouse input
- selection

- font size is wrong, not taken from settings it seems
- font setSize & other setters should perhaps be renamed to withSize, etc to avoid confuision about them being setters
- size.width() & size.height() should become just width and height
- some simpler accesses to state-ish properties in the terminal? 


- document keyboard focus handling - i.e. only document order and getting next & previous elements
- things like tab focus and so on should be implemented differently, perhaps by a form widget or some such




- cursor visibility determined by its position too
- ansi terminal palette in config.h

- for painting Windows can paint any time they want, X11's render will send the expose message (check that repaints are only possible in expose handlers) and for Qt I need to see what needs to be done
- x11_window seems to no longer need the focusIn and focusOut checks as vcxsrv seems to be fixed. Also check that renderer in release mode ignores the error and repairs what it can 

- locks.h - remove grabber & smartptr, make priority lock non-reentrant
- ansi-renderer should go to ansi-terminal
- renderer-widget coordinates, check that they are valid
- key up not detected on standard terminal, determine how tpp would do it
- mouse up event, detect click and double click
- add selection support to renderer
- ansi-terminal should use the ansi-keys, which should move to tpp-lib - perhaps even the whole terminal should move there
- CSI sequence moved to own file, should be removed from terminal

- document keyboard, mouse and selection & clipboard inputs in renderer & widgets
- document event queue and typed event queue

- resizing does not work in xterm

# Roadmap

> This section discusses plan for next versions and drafts the features planned. 

### v0.8

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



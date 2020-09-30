The following is very short description of the versions planned and their main new features. There are no set release dates, a release will happen once the planned features are implemented and tested a bit.

## Next version: 0.8

- alternate zoom shortcuts (`C-S--` and `C-S-=`)
- ropen can be interrupted gracefully
- proper single-thread UI rewrite, simpler & more robust UI
- multiple sessions can be specified in the configuration and switched with `--session` command-line argument
- improved Qt support (QWidget instead of QWindow-based renderer) (issue #10)
- fixed issue #20, `--cols` and `--rows` arguments & settings work now
- fixed issue #21, Device Status Report sequence support
- added option to specifyu clipboard setting behavior (#23)

### TODO

> These are items I currently work on towards the next version. When they are done, they die.  

Documentation:

- document keyboard focus handling - i.e. only document order and getting next & previous elements
- things like tab focus and so on should be implemented differently, perhaps by a form widget or some such
- document keyboard, mouse and selection & clipboard inputs in renderer & widgets

Builds & Releases

- source package artifact for deb
- release ropen package to snap 
- automatic release to Windows store
- upload to ppa ? 
- update the website ? 
- add benchmarking
- revisit how errors are reported and logged and how things are checked before releases

Helpers & Logging & Errors overhaul

- helpers filesystem should use paths better

Issues to be raised:

- add a way to copy from mouse capturing terminal
- how to & when invalidate selection when there are changes in the terminal's contents

UI version 3

- when scrolling with selection update nothing gets updated (when widget gets scrolled, mouse move should be reissued because the mouse effectively moved - think this through)

- some simpler accesses to state-ish properties in the terminal? 


tpp-server

- key up not detected on standard terminal, determine how tpp would do it
- resizing does not work in xterm on WSL

# Roadmap

> This section discusses plan for next versions and drafts the features planned. 

### v0.9

- some basic styling for the GUI via a style object, similar to CSS?, more powerful buttons & labels
- color for dimming disabled terminal (via styling)
- scrollview, tooltips, shortcuts and actions and some more UI elements
- tooltip widget can be obtained by renderer and renderer knows its rectangle and can therefore issue repaints as appropriate
- PositionHint: layout / absolute - absolute positioned elements are not updated by layouts
- keyboard focus with tabIndices (this means that the getNextFocusableWidget has to be patched to look for the focusable widgets according to their indices instead of position in the children vector)

- WidgetBorder should only require repaint of children that interfere with the border (can be calculated from their visible rects)
- buffering relayouts (see Layout::setChildOverlay)

- geometry.h is areally bad name for some of the classes defined therein


### v0.10

- even more UI elements
- multiple sessions (in same window)

### v0.11

- multiple sessions in multiple windows or tiling
- better bug reporting support and optional checking of updates for non-store installers
- systray support & platform notifications

### v0.12

- proper multiplexing of the terminal sessions (wrt remote files)
- remote files should use absolute path, or some other form of same file detection

### v1.0

- polish and bugfixing of stuff, some beta releases and "testing"
- remove the tpp application from snap (tpp.yaml)

# Unassigned Features

> These are features that I am considering to add, but no immediate plans to do so, noty even sure if they will happen.  

- keyboard shortcuts
  - when implemented, add keyDown in selection owner to deal with selection & clipboard
- UI actions
- add zoom out? and default zoom? 
- xim is wrong, it works on linux, but fails on mac
- how to deal with palette? (like a global palette object, and being able to set palette mappings for the widgets? 
- add transparency for entire window? on Windows - https://msdn.microsoft.com/en-us/magazine/ee819134.aspx?f=255&MSPPError=-2147217396 and use layered windows



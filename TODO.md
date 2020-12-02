The following is very short description of the versions planned and their main new features. There are no set release dates, a release will happen once the planned features are implemented and tested a bit.

## Next version:

- hyperlinks (`http` and `https` links can be autodetected, `OSC 8` escape sequence supported as well, configurable styles)
- double click selects word, triple click selects line
- option to disable cursor style changes from terminal apps
- selection contents is trimmed
- renderer can use different mouse cursors
- proper mouse coordinates are reported when history is used together with buffer
- easier transition to newer compatible versions
- enter acts on buttons and other minor UI improvements
- single tpp-bypass for all WSL distributions

### TODO

> These are items I currently work on towards the next version. When they are done, they die.  

- add Visul Studio Dev Prompt to sessions

- see if msys can be added to bypass

Documentation:

- document keyboard focus handling - i.e. only document order and getting next & previous elements
- things like tab focus and so on should be implemented differently, perhaps by a form widget or some such
- document keyboard, mouse and selection & clipboard inputs in renderer & widgets

Builds & Releases

- source package artifact for deb
- release ropen package to snap 
- upload to ppa ? 
- update the website ? 
- add benchmarking
- revisit how errors are reported and logged and how things are checked before releases

Helpers & Logging & Errors overhaul

- helpers filesystem should use paths better

Issues to be raised:

- add a way to copy from mouse capturing terminal
- how to & when invalidate selection when there are changes in the terminal's contents

UI 

- when scrolling with selection update nothing gets updated (when widget gets scrolled, mouse move should be reissued because the mouse effectively moved - think this through)

- some simpler accesses to state-ish properties in the terminal? 

- how to deal with dialogs being deleted after dismiss. Have something like a forced schedule, which associates with renderer, but actually executes all the actions when the renderer dies, or actually execute all renderer actions when renderer dies? 

Better Terminal 

- adjustCursorPosition in buffer should be reused by terminal
- terminal has no history, that is done by its subclasses, i.e. `HistoryTerminal<AnsiTerminal>` and so on
- and so does the scrolling...

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
- add transparency for entire window? on Windows - https://msdn.microsoft.com/en-us/magazine/ee819134.aspx?f=255&MSPPError=-2147217396 and use layered windows
  on Linux this is almost ready by switching to ARGB visual as part of searching for ways to draw transparent rectangles (perhaps we want this to be turnable on/off - see commit https://github.com/terminalpp/terminalpp/commit/22e4ea42c8496e7e0ac842df5f33fc1e44af9945)





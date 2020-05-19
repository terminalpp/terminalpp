The following is very short description of the versions planned and their main new features. There are no set release dates, a release will happen once the planned features are implemented and tested a bit.

## Next version: v0.7

> This is the next release target and things that have already changed. When released this is copied to the CHANGELOG.md

More robust UI framework, better configuration options and slightly increased security.

- double width and double size fonts dropped from the terminal (not from the UI)
- history gets resized when terminal gets resized
- inactive cursor properly displayed

### TODO

> These are items I currently work on towards the next version. When they are done, they die.  

- when dismissing the about box by a key, the key still goes to the terminal

- check that the detachment of thread in helpers::time.h is safe

- FPS, repaint rate and other stats

- button basics, label fonts

- geometry.h is areally bad name for some of the classes defined therein

- update log - verbose and debug logs

Terminal++ PTY:

- the bypass is quite slow, even locally, bottlenecks should be investigated
- store to altenrate path if can't be opened (or ask?)
- do Sequence::Response<> that is either the response, or NACK
- check with tmux - seems okish
- add more properties to ropen

- PTY should have special provision for sending Sequences

Settings:

- design settings via abstract configurator classes that provide the interface
- do settings for terminal history, including history, or shared history for the alternate buffer if requested
- change settings to reflect other changes (cursors etc)
- color for inactive cursor in terminal
- color for dimming disabled terminal
- add configuration option to confirm setting clipboard from the terminal

Documentation:

- write documentation for layouts and revisit if I want to do anchors (perhaps via a special anchor layout, or via Layout::None that would just notify the child that change has occured) - most likely I do not want this... 
- document size hints 
- add comments to keyboard focus stuff
- mouseUp, Down, WHeel, Move, Click and DoubleClick can propagate to parent, update the documentation
- and so does the keyboard, propagation to parent disabled by default if event enabled 
- document the modal pane & modal trait

Builds & Releases

- snapcraft build has dirty stamp, see why & fix
- ubuntu ppa
- change from MSI to appx
- add benchmarking
- revisit how errors are reported and logged and how things are checked before releases

Issues to be raised:

- add a way to copy from mouse capturing terminal
- how to & when invalidate selection when there are changes in the terminal's contents
- fillRect should also update border color

# Roadmap

> This section discusses plan for next versions and drafts the features planned. 

### v0.8

- some basic styling for the GUI via a style object, similar to CSS?
- scrollview, tooltips, shortcuts and actions and some more UI elements
- tooltip widget can be obtained by renderer and renderer knows its rectangle and can therefore issue repaints as appropriate
- PositionHint: layout / absolute - absolute positioned elements are not updated by layouts
- keyboard focus with tabIndices (this means that the getNextFocusableWidget has to be patched to look for the focusable widgets according to their indices instead of position in the children vector)

- WidgetBorder should only require repaint of children that interfere with the border (can be calculated from their visible rects)


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
- change from msi to appx, add windows store submission if possible 

# Unassigned Features

> These are features that I am considering to add, but no immediate plans to do so, noty even sure if they will happen.  

- keyboard shortcuts
- UI actions
- add zoom out? and default zoom? 
- xim is wrong, it works on linux, but fails on mac
- how to deal with palette? (like a global palette object, and being able to set palette mappings for the widgets? 
- add transparency for entire window? on Windows - https://msdn.microsoft.com/en-us/magazine/ee819134.aspx?f=255&MSPPError=-2147217396 and use layered windows



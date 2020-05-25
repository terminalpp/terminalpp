The following is very short description of the versions planned and their main new features. There are no set release dates, a release will happen once the planned features are implemented and tested a bit.

## Next version: v0.7

> This is the next release target and things that have already changed. When released this is copied to the CHANGELOG.md

More robust UI framework, better configuration options and slightly increased security.

- double width and double size fonts dropped from the terminal (not from the UI)
- history gets resized when terminal gets resized
- inactive cursor properly displayed
- modal errors in terminal

### TODO

> These are items I currently work on towards the next version. When they are done, they die.  

- configuration settings will determine the channel which to check for new versions
- display the notification always? or dismiss for given version? 
- add timeouts for notifications ? 
- add styling to dialogs (error, warning, notice, primary, a la bootstrap)

- logging to files
- make sure that AppData and other folders are accessed correctly from msix
- Ubuntu 20.04 bypass

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
- add benchmarking
- revisit how errors are reported and logged and how things are checked before releases

Change from MSI to MSIX

- MakeAppx.exe pack /d . /p terminalpp.msix
- do this in the folder where the package will be created 
- copy terminalpp
- copy icons and so on
- how to sign? 

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

- some basic styling for the GUI via a style object, similar to CSS?, more powerful buttons & labels
- color for dimming disabled terminal (via styling)
- scrollview, tooltips, shortcuts and actions and some more UI elements
- tooltip widget can be obtained by renderer and renderer knows its rectangle and can therefore issue repaints as appropriate
- PositionHint: layout / absolute - absolute positioned elements are not updated by layouts
- keyboard focus with tabIndices (this means that the getNextFocusableWidget has to be patched to look for the focusable widgets according to their indices instead of position in the children vector)

- WidgetBorder should only require repaint of children that interfere with the border (can be calculated from their visible rects)

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
- change from msi to appx, add windows store submission if possible 

# Unassigned Features

> These are features that I am considering to add, but no immediate plans to do so, noty even sure if they will happen.  

- keyboard shortcuts
- UI actions
- add zoom out? and default zoom? 
- xim is wrong, it works on linux, but fails on mac
- how to deal with palette? (like a global palette object, and being able to set palette mappings for the widgets? 
- add transparency for entire window? on Windows - https://msdn.microsoft.com/en-us/magazine/ee819134.aspx?f=255&MSPPError=-2147217396 and use layered windows



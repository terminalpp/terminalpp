The following is very short description of the versions planned and their main new features. There are no set release dates, a release will happen once the planned features are implemented and tested a bit.

## Next version: v0.7

> This is the next release target and things that have already changed. When released this is copied to the CHANGELOG.md

More robust UI framework, better configuration options and slightly increased security.

- double width and double size fonts dropped from the terminal (not from the UI)
- history gets resized when terminal gets resized
- inactive cursor properly displayed

### TODO

> These are items I currently work on towards the next version. When they are done, they die.  

- WidgetBorder should only require repaint of children that interfere with the border (can be calculated from their visible rects)

- add comments to layout
- write documentation for layouts and revisit if I want to do anchors (perhaps via a special anchor layout, or via Layout::None that would just notify the child that change has occured) - most likely I do not want this... 
- modal widgets & keyboard focus

- revisit & update how widget children are drawn and how/when their visible rectangles are updated and whether the Container::Add works properly

- directwrite & X11 & qFont check double size font is working (seemslike the calculation ignores the font size)
- qfont should keep the width if set

- PTY++ and remote files version 2.0 - or maybe just do the old PTY but in PTY++ pty in the new architecture.
- deal with how PTYs are deleted (and the entire session)

- hide console window in windows (arbitrary make the window visible on a configuration condition)

- design settings via abstract configurator classes that provide the interface
    - do settings for terminal history, including history, or shared history for the alternate buffer if requested
- change settings to reflect other changes (cursors etc)

- shapes.h is areally bad name for the classes defined therein

- add benchmarking
- revisit how errors are reported and logged and how things are checked before releases

- snapcraft build has dirty stamp, see why & fix
- ubuntu ppa distribution
- change from msi to appx (this would also remove the dependency on .NET 3.5 for wix)

Issues to be raised:

- add configuration option to confirm setting clipboard from the terminal
- add a way to copy from mouse capturing terminal
- how to & when invalidate selection when there are changes in the terminal's contents
- fillRect should also update border color

# Roadmap

> This section discusses plan for next versions and drafts the features planned. 

### v0.8

- multiple sessions (in same window)
- when multiple sessions are supported, the RendererWindow can set the font instead of the subclasses

### v0.9

- multiple sessions in multiple windows or tiling
- better bug reporting support and optional checking of updates for non-store installers
- systray support & platform notifications

### v0.10

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



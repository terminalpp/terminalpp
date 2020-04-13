# Known Bugs

### Win32 Bugs

### X11 Bugs

- transparency of the background does not seem to work

# Roadmap

The following is very short description of the versions planned and their main new features. There are no set release dates, a release will happen once the planned features are implemented and tested a bit.

### v0.7

- double width and double size fonts dropped from the terminal (not from the UI)
- history gets resized when terminal gets resized
- inactive cursor properly displayed

##### TODO

- rendererClose is invoked before all user events are processed and therefore segfaults on already deleted window - figure out how to fix this

- design proper and simple layouts and autosizing of widgets
- revisit & update how widget children are drawn and how/when their visible rectangles are updated and whether the Container::Add works properly

- directwrite & X11 & qFont check double size font is working (seemslike the calculation ignores the font size)
- qfont should keep the width if set

- PTY++ and remote files version 2.0
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

- mouseOut is not reported at all in QT
- add configuration option to confirm setting clipboard from the terminal
- add a way to copy from mouse capturing terminal
- how to & when invalidate selection when there are changes in the terminal's contents
- fillRect should also update border color

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

### Later

- keyboard shortcuts, actions, better settings than JSON, etc. 

## Non-version TODOs

- add zoom out? and default zoom? 
- add JSON schema and validator support
- allow container to reverse order of its elements (so that maximized and horizontal layouts can be used nicely with header lines both up and down)
- onChange handler for generic changes
- make canvas do more
- xim is wrong, it works on linux, but fails on mac
- how to deal with palette? (like a global palette object, and being able to set palette mappings for the widgets? 
- add transparency for entire window? on Windows - https://msdn.microsoft.com/en-us/magazine/ee819134.aspx?f=255&MSPPError=-2147217396 and use layered windows



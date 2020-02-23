# Known Bugs

### Win32 Bugs

### X11 Bugs

- transparency of the background does not seem to work

# Roadmap

The following is very short description of the versions planned and their main new features. There are no set release dates, a release will happen once the planned features are implemented and tested a bit.

### v0.6

- simple tests framework in helpers, tests target added
- reentrant lock in helpers
- simpler events (single handler, std::function, method and fptr handlers)
- configuration code refactoring
- simpler UI code
- more automation for releases
- build fixed so that stamp is only generated when required
- better errors for invalid JSON settings

##### TODO

- resize the contents label and alert panel when shown
- add option that tells when to show the paste confirmation
- confirmation box for paste and for remote files (see https://thejh.net/misc/website-terminal-copy-paste)

- design proper and simple layouts and autosizing of widgets

- how to deal with cursor of inactive terminal in active window - this seems to lead to multiple cursors per window and how to handle them... (?)

- simpler UI framework
- remove / refactor builders
- shapes.h is areally bad name for the classes defined therein

- snapcraft build has dirty stamp, see why & fix
- change from msi to appx

- add benchmarking
- revisit how errors are reported and logged and how things are checked before releases

### v0.7

- multiple sessions (in same window)

### v0.8

- multiple sessions in multiple windows or tiling
- better bug reporting support and optional checking of updates for non-store installers
- systray support & platform notifications

### v0.9

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




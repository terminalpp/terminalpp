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

##### TODO

- confirmation box for paste and for remote files
- lineOut should center/top/bottom vertically too
- simpler UI framework
- remove / refactor builders
- shapes.h is areally bad name for the classes defined therein

- create proper documentation and figure out how to release it
- add Windows Store Submission - can this be automated in any way? 
- add benchmarking

### v0.7

- add QT backend that will work on Mac too (maybe use wxWidgets?)
- is there a mac store? 

### v0.8

- systray support
- multiple sessions
- proper multiplexing of the terminal sessions (wrt remote files)

### v1.0

- polish and bugfixing of stuff, some beta releases and "testing"
- remove the tpp application from snap (tpp.yaml)

### Later

- keyboard shortcuts, actions, better settings than JSON, etc. 

## Non-version TODOs

- add zoom out? and default zoom? 
- add JSON schema and validator support
- allow container to reverse order of its elements (so that maximized and horizontal layouts can be used nicely with header lines both up and down)
- onChange handler for generic changes
- make canvas do more
- xim is wrong, it works on linux, but fails on mac
- then at some point in the future, helpers, tpp and ui+vterm should go in separate repos
- how to deal with palette? (like a global palette object, and being able to set palette mappings for the widgets? 
- add transparency for entire window? on Windows - https://msdn.microsoft.com/en-us/magazine/ee819134.aspx?f=255&MSPPError=-2147217396 and use layered windows




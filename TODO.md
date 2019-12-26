# Known Bugs

### Win32 Bugs

### X11 Bugs

- transparency of the background does not seem to work

# Roadmap

The following is very short description of the versions planned and their main new features. There are no set release dates, a release will happen once the planned features are implemented and tested a bit.

### v0.6

- cursor appearance can be specified in `settings.json`

- simple tests framework in helpers, tests target added
- reentrant lock in helpers
- simpler events (single handler, std::function, method and fptr handlers)

##### TODO

- simpler UI framework
- Windows store submission
- more automation for releases

Questions:

- delete mousecoords from root window
- window to widget coordinates in terminal (and scrollable widgets in general)

- should Container's attach & detach children be virtual? 
- create proper documentation and figure out how to release it
- add Windows Store Submission - can this be automated in any way? 
- add benchmarking

- simpler settings where not everything will be stored and only changes from defaults will be loaded

### v0.7

- systray support
- multiple sessions
- proper multiplexing of the terminal sessions (wrt remote files)

### v1.0

- polish and bugfixing of stuff, some beta releases and "testing"

### Later

- keyboard shortcuts, actions, better settings than JSON, etc. 

## Non-version TODOs

- ssh plain does not display boxes in mc well
- add showModal(Widget *) to root window, which would dim the background and then display the widget centered
- add zoom out? and default zoom? 
- add JSON schema and validator support
- allow container to reverse order of its elements (so that maximized and horizontal layouts can be used nicely with header lines both up and down)
- onChange handler for generic changes
- make canvas do more
- xim is wrong, it works on linux, but fails on mac
- then at some point in the future, helpers, tpp and ui+vterm should go in separate repos
- how to deal with palette? (like a global palette object, and being able to set palette mappings for the widgets? 
- add transparency for entire window? on Windows - https://msdn.microsoft.com/en-us/magazine/ee819134.aspx?f=255&MSPPError=-2147217396 and use layered windows

# Changelog

> These are the versions already released and their major changes

### v0.5 - Remote File Opening

- session palette can be customized in settings.json
- terminal wait on terminated PTY is customized in settings.json
- Ubuntu WSL w/o version in name supported for bypass installation
- bold text can be forced to render in bright colors
- support for opening remote files by extra escape sequences (basic)
- snap store at edge

### v0.4

- support terminal history, scrolling, selection from it, etc. 
- will open `settings.json` for editing when requested

### v0.3

- font fallback, CJK, double width & height fonts


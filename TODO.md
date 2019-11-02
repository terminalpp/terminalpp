# Known Bugs

### Win32 Bugs

### X11 Bugs

- transparency of the background does not seem to work

# Roadmap

The following is very short description of the versions planned and their main new features. There are no set release dates, a release will happen once the planned features are implemented and tested a bit.

### v0.5

- session palette can be customized in `settings.json`
- terminal wait on terminated PTY is customized in `settings.json`
- bold text can be forced to render in bright colors
- support for opening remote files by extra escape sequences (basic)

- update documentation generation into the website
- ideally snap store will be vetted by this time
- Windows Store version as well

Maybe not necessary now: 

- the tpp terminal should use the `<<` and `>>` notation so that it can be used instead of std::cout & std::cin

### v0.6

- multiple sessions

### v0.7

- systray support

### v1.0

- polish and bugfixing of stuff, some beta releases and "testing"

### Later

- keyboard shortcuts, actions, better settings than JSON, etc. 

## Non-version TODOs

- ssh plain does not display boxes in mc well
- add showModal(Widget *) to root window, which would dim the background and then display the widget centered
- clean shapes - rect, to provide the points for corners on demand
- add zoom out? and default zoom? 
- improve logging - the logging overhead is gigantic, find a way to lower it
- add JSON schema and validator support
- add tests
- allow container to reverse order of its elements (so that maximized and horizontal layouts can be used nicely with header lines both up and down)
- onChange handler for generic changes
- make canvas do more
- xim is wrong, it works on linux, but fails on mac
- then at some point in the future, helpers, tpp and ui+vterm should go in separate repos
- how to deal with palette? (like a global palette object, and being able to set palette mappings for the widgets? 
- add transparency for entire window? on Windows - https://msdn.microsoft.com/en-us/magazine/ee819134.aspx?f=255&MSPPError=-2147217396 and use layered windows



# Changelog

> These are the versions already released and their major changes

### v0.4

- support terminal history, scrolling, selection from it, etc. 
- will open `settings.json` for editing when requested

### v0.3

- font fallback, CJK, double width & height fonts


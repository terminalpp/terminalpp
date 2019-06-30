# TODOs

- priority synchronization should be done by condition variable instead of busy waiting
- and cruicially, is the priority locking even necessary? 

## Bugs and missing features

- switch char to unly use UTF8 and provide translations to other encodings
- proper UTF8 and UTF32 conversion in helpers::strings


### Win32 Bugs

### X11 Bugs

- fullscreen does not work

# Code Fixes

> These are important and should be done before release

- bypass should quickly scan for ` and only do stuff if found, because most of the time it won't be


# Code Improvements 

- screen should have scrolling functions in itself (?) - I guess yes...
- improve logging - the logging overhead is gigantic, find a way to lower it
- add tests & CI


# Long Term Goals

- non-english double width characters don't work properly (at all)
- vt100 should have palette access ready

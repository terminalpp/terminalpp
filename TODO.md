# TODOs

- priority synchronization should be done by condition variable instead of busy waiting

## Bugs and missing features

- clearing the buffer does not seem to be enough, better still, last character in a row should always clear the stuff right off
- proper UTF8 and UTF32 conversion in helpers::strings
- when terminal is resized, try to keep stuff in the buffer

### Win32 Bugs

### X11 Bugs

- fullscreen does not work

# Code Fixes

> These are important and should be done before release

- asciienc should quickly scan for ` and only do stuff if found, because most of the time it won't be
- it should be renamed and code documented properly


# Code Improvements 

- improve logging - the logging overhead is gigantic, find a way to lower it
- switch char to unly use UTF8 and provide translations to other encodings
- add tests & CI


# Long Term Goals

- non-english double width characters don't work properly (at all)

- what if backend and terminal are put together and they use only PTY as extra step. This simplifies the process great deal... 



- vt100 should have palette access ready
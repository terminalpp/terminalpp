# TODOs

# all platforms

- clearing the buffer does not seem to be enough, better still, last character in a row should always clear the stuff right off
- proper UTF8 and UTF32 conversion in helpers::strings
- when terminal is resized, try to keep stuff in the buffer
- implement FPS-style rendering

# Win32

# X11

- fullscreen does not work

# Long Term Goals

- non-english double width characters don't work properly (at all)

# Code Improvements 

- improve logging
- switch char to unly use UTF8 and provide translations to other encodings
- add tests & CI
- asciienc should quickly scan for ` and only do stuff if found, because most of the time it won't be
- it should be renamed and code documented properly

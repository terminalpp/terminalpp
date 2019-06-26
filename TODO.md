# TODOs

# all platforms

- underline and strikeout are not working
- clearing the buffer does not seem to be enough, better still, last character in a row should always clear the stuff right off
- proper UTF8 and UTF32 conversion in helpers::strings
- when terminal is resized, try to keep stuff in the buffer
- asciienc is useless honestly... there is no point in encoding, it is only the bypass PTY that we need on Windows. 

# Win32

# X11

- fullscreen does not work

# Code Improvements 

- improve logging
- switch char to unly use UTF8 and provide translations to other encodings
- add tests & CI
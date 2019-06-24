# TODOs

# all platforms

- simpler font size selection (only height, calculate width from font)
- ctrl-v capture means it is unusable
- blinking text is not dirty for now
- verify that bold, italics and underline works (as well as strikethrough) (underline and strikethrough won't imho for now)
- cursor should not be displayed, or be displayed differently when window is out of focus
- clearing the buffer does not seem to be enough, better still, last character in a row should always clear the stuff right off
- proper UTF8 and UTF32 conversion in helpers::strings
- when terminal is resized, try to keep stuff in the buffer

# Win32

- check font sizes when zoom active

# X11

# Code Improvements 

- improve logging
- switch char to unly use UTF8 and provide translations to other encodings
- add tests & CI
# TODOs

# all platforms

- setting very large zoom on small window hits assertion
- blinking text is not dirty for now
- verify that bold, italics and underline works (as well as strikethrough) (underline and strikethrough won't imho for now)
- clearing the buffer does not seem to be enough, better still, last character in a row should always clear the stuff right off
- proper UTF8 and UTF32 conversion in helpers::strings
- when terminal is resized, try to keep stuff in the buffer
- behaves weirdly if terminal size is too small (such as when zoom max)

# Win32

# X11

- fullscreen does not work

# Code Improvements 

- improve logging
- switch char to unly use UTF8 and provide translations to other encodings
- add tests & CI
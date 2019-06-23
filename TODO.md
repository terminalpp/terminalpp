# TODOs

- blinking text is not dirty for now
- cursor should not be displayed, or be displayed differently when window is out of focus
- clearing the buffer does not seem to be enough, better still, last character in a row should always clear the stuff right off
- when terminal is resized, try to keep stuff in the buffer
- improve logging
- proper UTF8 and UTF32 conversion in helpers::strings
- improve font sizes in X11 (this should also get rid of ghosts in X11)
- switch char to unly use UTF8 and provide translations to other encodings

# ASCII Encoder

The `ASCII` encoder is a simple application that takes as its arguments the app to execute and its arguments, executes the application and encodes all of its input and output to a printable ASCII characters only. 

This is useful mostly for Windows where the `conpty` pseudoterminal swallows some escape sequences (notably mouse) and similar situations since the printable ASCII codes have never (AFAIK) extra meaning.

## Encoding Scheme


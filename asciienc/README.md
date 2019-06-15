# ASCII Encoder

The `ASCII` encoder is a simple application that takes as its arguments the app to execute and its arguments, executes the application and encodes all of its input and output to a printable ASCII characters only. 

This is useful mostly for Windows where the `conpty` pseudoterminal swallows some escape sequences (notably mouse) and similar situations since the printable ASCII codes have never (AFAIK) extra meaning.

## Encoding Scheme

The encoding uses only characters from ` ` (space, 0x20) to `~` (tilde, 0x7f). All characters within this range with the exception of a `` ` `` (backtick, 0x60) are kept as they are. 

The backtick functions as an escape character. Its immediate successor then determines the encoding scheme:

- ``` `` ``` (double backtick) translates to a single backtick on the output
- `` `@ `` - `` `_ `` (i.e. followed by characters from 0x40 to 0x5f) are translated as characters from the range 0x00 - 0x1f
- `` `hh `` where `h` stands for a hex digit (`0..9` and `a..f`) translates to a character with code corresponding to the hex value
- `` `x `` means a special command (see chapter below). All commands must end with `;` (semicolon)

## Commands

The following commands are supported:

### Resize

Resize command is sent to the client by the terminal when the terminal should be resized. The command structure is as follows:

    `xr<cols>:<rows>;

Where `<cols>` and `<rows>` stand for new number of columns and rows respectively, both encoded as decimal numbers. As an example `` `xr80:25; `` should set the terminal to `80` columns and `20` rows. 

## Known Limitations

When used with `wsl`, the `SHELL` environment variable is set to `asciienc` executable path instead of `/bin/bash` or some such. This causes veird problems for many programs, such as `tmux` not starting properly, *some* ssh connections not working, or `mc` failing to launch commands. All can be fixed by explicitly changing the `SHELL` variable to bash. 



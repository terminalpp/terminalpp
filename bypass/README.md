# ConPTY Bypass

Because the Windows `conpty` pseudoterminal swallows some escape sequences (notably mouse), the `bypass` is a very simple application intended to be executed inside `wsl` locally, which creates a linux pseudoconsole directy. It then translates the input and output to normal non-console standard in and out stream, which are not subject to the `conpty` processing.

## Encoding Scheme

## Commands

All commands start with `` ` ``. If backtick is to be transmitted, then double backtick (``` `` ```) is transmitted instead. Otherwise a command is identified by the character following the backtick: 

### `r` - Resize

Resize command is sent to the client by the terminal when the terminal should be resized. The command structure is as follows:

    `r<cols>:<rows>;

Where `<cols>` and `<rows>` stand for new number of columns and rows respectively, both encoded as decimal numbers. As an example `` `r80:25; `` should set the terminal to `80` columns and `20` rows. 

## Known Limitations

When used with `wsl`, the `SHELL` environment variable is set to `asciienc` executable path instead of `/bin/bash` or some such. This causes veird problems for many programs, such as `tmux` not starting properly, *some* ssh connections not working, or `mc` failing to launch commands. All can be fixed by explicitly changing the `SHELL` variable to bash. 



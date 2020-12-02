# ConPTY Bypass

Because the Windows `conpty` pseudoterminal swallows some escape sequences (notably mouse), the `tpp-bypass` is a very simple application intended to be executed inside `wsl` locally, which creates a linux pseudoconsole directly. It then translates the input and output to normal non-console standard in and out stream, which are not subject to the `conpty` processing.

The only thing needed to build the program is the `tpp-bypass.cpp` file in this folder so that it can be easily embedded in the terminal and installed when appropriate. 

## Installation

To install the bypass, make sure you have `root` access (required for installing into default system locations) and then run the following in the WSL prompt:

    sudo apt install cmake g++ git
    git clone https://github.com/zduka/tpp-bypass.git
    cd tpp-bypass
    ./install.sh
    
After this the `tpp-bypass` program will be installed to your path and you can use it with `tpp`.

## Usage

> `t++` users don't need to worry about the bypass as it is transparently invoked by the terminal when configured. 

    tpp-bypass { --buffer-size N | envVar=value} [ -e cmd { arg}]

where:

- `--buffer-size N` sets the I/O buffers of the terminal to `N` bytes
- `envVar=value` adds the `envVar` environment variable to the target command environment and sets it to the `value`
- `-e cmd {args}` tells bypass to execute the given command with specified attributes. The default command is the current user's default shell.  

## Known Limitations

By default, the `SHELL` environment variable will be set to `bypass`, which would cause errors for any programs launching shells as they would try to launch the `bypass` instead. Unless the `SHELL` variable is manually set by the commandline, `bypass` sets it to the current user's default shell instead. 

## Encoding Scheme

The encoding scheme is very primitive. All target command output is passed directly to the terminal unchanged. Any input from the terminal to the command is scanned for commands, these are performed and the rest of the input is sent as input to the target command. 

All bypass commands are prefixed with a backtick `` ` ``. If backtick is to be transmitted, then double backtick (``` `` ```) is transmitted instead. Otherwise a command is identified by the character following the backtick. The following commands are supported: 

### `r` - Resize

Resize command is sent to the client by the terminal when the terminal should be resized. The command structure is as follows:

    `r<cols>:<rows>;

Where `<cols>` and `<rows>` stand for new number of columns and rows respectively, both encoded as decimal numbers. As an example `` `r80:25; `` should set the terminal to `80` columns and `20` rows. 

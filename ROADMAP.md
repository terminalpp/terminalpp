# Terminal++ Roadmap

The ultimate goal of `terminalpp` is to bring terminals to 21st century:) From the very beginning the project has been designed with crossplatform compatibility and modularization in mind. 

`terminalpp` extends the existing ANSI escape codes by additional useful commands (such as opening remote files from the terminal by transmitting them over the existing terminal connection). This would lead to proper terminal multiplexing where single connection, be it local/ssh/anything else will be able to transmit multiple pseudoterminals, file transfers, etc. 

In the future, `terminalpp` should be extended with its own server-side terminal multiplexer (`tmux` replacement) and interactive shell ("`bash`" replacement:) that while remaning reasonably backwards compatible will make use of the advanced `terminalpp` capabilities. 

All of these tools should however work equally well with basic terminals and the `terminalpp` itself should always be first and foremost a crossplatform terminal emulator. 

## Current Status

- `terminalpp` works on Windows, Linux and macOS
- most of the subset of `xterm-256` terminfo that is used by contemporary tools such as `tmux`, `emacs`, `vim`, `mc` and so on is working
- bi-directional clipboard
- basic support for read-only opening of remote files
- althout WSL, cmd.exe and powershell all work, switching between them is very user unfriendly
- single session only

## v1.0

- multiple sessions in the configuration file (WSL, cmd.exe, powershell, different linux shells, etc)
- multiple currently running sessions (not yet sure how to switch between them, tabs? helm-like?, multiple windows?)
- keyboard shortcuts and their configuration
- UI polish
- better integration with the OS (systray, notifications)

## v2.0

- proper terminal multiplexing
- split into different projects (ui, ropen, ...)

## Not yet Assigned

- termux-like Android version
- multiplexer
- shell


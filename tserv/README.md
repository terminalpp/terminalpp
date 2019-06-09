# Terminal Server

For now the terminal server is a simple app that executes given process and then relays its input and output on the provided TCP port. 

This is now very useful for Windows scenario when `conpty` stands between the terminal and its application and several escape sequences (such as mouse movement) can be mangled, or completely swallowed. Running `tserv` from `wsl` and then connecting to its port bypasses the `conpty`. 

> Note that for now, `tserv` is only intended for Linux use and is not part of the build process in Windows.

> In the perhaps likely future, the `tserv` will become a `tmux` replacement operating on the new encoding scheme allowing better utilization of the `t++` features.


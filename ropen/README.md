# `ropen` - Opening Remote Files

`ropen` sends local file to the attached terminal, after which the file will be opened on the machine which hosts the terminal itself. `ropen` is also able to bypass `tmux` along the way. 

> Only a single instance of `tmux` can be bypassed for any given connection. This should not be much of a problem as `tmux` inside `tmux` is discouraged anyways. 

## Encoding

Because `BEL` character is used to terminate the OSC escape sequences used for the file transfer, this character must not be present in the output. This is done by 


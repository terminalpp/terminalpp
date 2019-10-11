# `ropen` - Opening Remote Files

`ropen` sends local file to the attached terminal, after which the file will be opened on the machine which hosts the terminal itself.


## Encoding

Because `BEL` character is used to terminate the OSC escape sequences used for the file transfer, this character must not be present in the output. This is done by 


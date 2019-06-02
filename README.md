# Terminal++

*Terminal++*, or `tpp` is minimalistic, simple to use virtual terminal that allows basic `VT100` emulation, similar to that of the windows console, as well as much more complex `++` mode which allows things like graphics to be displayed as well. 

> This looks like a good font: http://typeof.net/Iosevka/ - it is also part of nerdfonts, which is the same font, but augmented with many extra symbols. Iosevka is by default not truly monospace between regular and bold variants, but both can be downloaded and installed at the same time, which fixes the issue.


# Platform Support

## Windows

## Linux

    sudo apt install libx11-dev libxft-dev

## WSL

First follow the linux rules above, then make sure that X server, such as `vcxsrv` is running and add the following to `.bashrc` or so:

    export DISPLAY=localhost:0.0

> Using this allows also X11 tunelling when using `ssh`. 

## Links to study:

https://github.com/Microsoft/console/tree/master/samples/ConPTY/EchoCon

## TODOs

> Determine how the terminal process & terminal work - is it safe to ignore anything a process sends when it is not connected to a terminal, since reconnecting will repaint it anyways? 
> or can we say that process & terminal are firmly attached? 

> When dealing with fonts, sometimes the bold font is thicker than normal font, which sounds rather bad. Also, nerd fornts do not behave like proper monospace fonts, which is bad too. 

> Refactor terminal window so that the renderer specific details are decoupled from the virtual terminal maintenance.

## Architecture Overview

To maximize cross-compatibility between various operating systems and rendering engines, `tpp` splits into different parts all briefly explained here:

### Virtual Terminal

The virtual terminal (`vterm`) describes the internal state of the terminal and holds all the data required to display its contents. 

### Terminal Window

Terminal window is a simple window which is attached to one virtual terminal. The terminal window displays the virtual terminal contents.

### Terminal Application

The terminal application is a collection of virtual terminals and terminal windows and manages the mapping between these. It also contains an extra virtual terminal displaying its own settings & control application. 







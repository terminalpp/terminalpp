# Terminal++

*Terminal++*, or `tpp` is minimalistic, simple to use virtual terminal that allows basic `VT100` emulation, similar to that of the windows console, as well as much more complex `++` mode which allows things like graphics to be displayed as well. 

> This looks like a good font: http://typeof.net/Iosevka/ - it is also part of nerdfonts, which is the same font, but augmented with many extra symbols. Iosevka is by default not truly monospace between regular and bold variants, but both can be downloaded and installed at the same time, which fixes the issue.


# Platform Support

## Windows

To build under Windows, install [Visual Studio](https://visualstudio.microsoft.com), then open the folder and select `tpp.exe` from cmake targets. 

> Tested with VS Community 2019. VS Code should work as well, as should command line tools (not tested). 

## Linux

A convenience script `linux.sh` installs the packages required to build `tpp` on the system as well as the required fonts. See the script for more details:

    bash linux.sh

When done, create the build directory and run `cmake`:

    mkdir build
    cmake ..
    make -j8

You can now execute the terminal by typing:

    tpp/tpp

> Tested on Ubuntu 18.04 LTS.

## WSL

`tpp` supports WSL and can be installed following the guide for Linux system above. 

> If `tpp` source code is shared between Windows and Linux, it may happen that running `cmake` will produce weird looking errors if the build directory is in Windows filesystem. A workaround is to create a build tree elsewhere (on linux FS) and specify path to it, such as:

    mkdir build
    cd build
    cmake /mnt/c/devel/tpp 
    make -j8

X server is obviously needed, [`vcxsrv`](https://sourceforge.net/projects/vcxsrv/) works fine so far. In order for it to work in WSL, add the following line to your profile:

    export DISPLAY=localhost:0.0

> Tested on WSL Ubuntu 18.04.

## Links to study:

https://github.com/Microsoft/console/tree/master/samples/ConPTY/EchoCon

## TODOs

> When dealing with fonts, sometimes the bold font is thicker than normal font, which sounds rather bad. Also, nerd fornts do not behave like proper monospace fonts, which is bad too. 

## Architecture Overview

To maximize cross-compatibility between various operating systems and rendering engines, `tpp` splits into different parts all briefly explained here:

### Virtual Terminal

The virtual terminal (`vterm`) describes the internal state of the terminal and holds all the data required to display its contents. 

### Terminal Window

Terminal window is a simple window which is attached to one virtual terminal. The terminal window displays the virtual terminal contents.

### Terminal Application

The terminal application is a collection of virtual terminals and terminal windows and manages the mapping between these. It also contains an extra virtual terminal displaying its own settings & control application. 







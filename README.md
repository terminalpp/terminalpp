# Terminal++

*Terminal++* or `tpp` is a minimalistic crossplatform terminal emulator that is capable of emulating much of the commonly used `xterm` supported control sequences. 

# Configuration

Configuration for now is very spartan. All configuration is done by changing macros in the `config.h` file. The option names should be fairly self explanatory. 

Note that if you provide wrong specification of the font (`DEFAULT_TERMINAL_FONT`), pty to use (`DEFAULT_SESSION_PTY`) or command to execute in the terminal (`DEFAULT_SESSION_COMMAND`) the terminal will fail in weird ways for now. 

## Fonts

`tpp` should work with any font the underlying renderer can work with, but has been tested extensively with `Iosevka` which was (for now) copied in the repo (see the `fonts/LICENSE.md` for the fonts license.

## Keybindings

These are default keybinding settings. They can be changed in the `config.h` as well:

`Ctrl +` and `Ctrl -` to increase / decrease zoom. 
`Ctrl Shift V` to paste from clipboard.
`Alt Space` to toggle full-screen mode.

# Platform Support

`tpp` currently supports Windows and Linux. See the notes below for details about respective platforms. 

> Ideally, MacOS will be supported at some time too. For now I have been told that if X server is installed, the Linux build instructions should work, but I have no means to test this myself. 

## Windows

To build under Windows, [Visual Studio](https://visualstudio.microsoft.com) and [cmake](https://cmake.org) must be installed. Open the Visual Studio Developer's prompt and get to the project directory. Then type the following commands:

    mkdir build
    cd build
    cmake ..
    cmake --build . --config Release

Then type the following to start the terminal:

    \tpp\Release\tpp.exe

### Using the bypass pseudoterminal

The implementation of Windows Pseudoterminal (ConPTY) actually reads the escape codes generated by the program and then reencodes them into own subset. While this preserves backwards compatibility and allows simple terminal emulators such as this one to provide frontends for `wsl`, `cmd`, `powershell` and more, it means that any bug or missing feature in this ConPTY translation cannot be fixed by the terminal emulator. This is most obvious by the inability to deal with mouse events (see actual [issue](https://github.com/microsoft/terminal/issues/545) on Windows Terminal repo for more details). 

To mitigate this, `tpp` has own bypass pseudoterminal, which uses `wsl` process to launch `wsl` pseudoterminal and then connects directly to its input and output bypassing the ConPTY. In order to use this feature, the bypass must be built in `wsl`. 

> This is currently very cumbersome, more proper versions should compile & install the bypass manually. 

To use the bypass, `tpp` must be built under `wsl` on the same machine and proper bypass path must be set in the `config.h` file. Follow the Linux installation instructions below for more information on how to build `tpp` in `wsl`. 

### Visual Studio Support

`tpp` can be opened as a folder in Visual Studio - tested in both Visual Studio Code and Visual Studio Community. The target to run is `tpp.exe`. 

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

### WSL Notes

`tpp` supports WSL and can be installed following the guide for Linux system above. 

> If `tpp` source code is shared between Windows and Linux, it may happen that running `cmake` will produce weird looking errors if the build directory is in Windows filesystem. A workaround is to create a build tree elsewhere (on linux FS) and specify path to it, such as:

    mkdir build
    cd build
    cmake PATH_TO_TPP_SOURCE # such as /mnt/c/projects/tpp 
    make -j8

X server is obviously needed, [`vcxsrv`](https://sourceforge.net/projects/vcxsrv/) works fine so far. In order for it to work in WSL, add the following line to your profile:

    export DISPLAY=localhost:0.0

> Tested on WSL Ubuntu 18.04.

# Performance

Although raw peformance is not the main design goal of `tpp`, it is reasonably performant under both Windows and Linux. The terminal framerate is limited (configurable, but defaults to 60 FPS) and per cell dirty flags are used so that unnecessary screen contents is not rendered. On Windows DirectWrite and Direct2D are used to render the characters while Linux implementation uses X11 and Xft for character rendering. 

Both seem to be more than efficient for daily operations. On Windows with bypass, `tpp` is way faster than all others, while on linux `tpp` is definitely one of faster terminals.

For lack of proper benchmarks, the [`vtebench`](https://github.com/jwilm/vtebench) from `alacritty` was used to test the basic performance. The `alt-screen-random-write`, `scrolling` and `scrolling-in-region` benchmarks are used because `tpp` does not support wide unicode characters yet. All the benchmarks were created at resolution `320x60` and the terminals rendered the `Iosevka Term` font at comparable sizes. 

> Take the following numbers with a big grain of salt since the `vtbench` provides extremely artificial benchmarks which only stress raw parser performance under certain conditions and full-screen rendering pauses at capped intervals (60 FPS to my understanding). Neither of these characteristics should really affect the day-to-day performance feeling for a terminal.  

## Windows 

On Windows, its main platform, `tpp` signifficantly outperforms the competition. This is partially due to reasonably efficient internal storage, reasonably fast rendering and most importantly due to complete bypass of the `ConPTY` (or `WinPTY`) console giving it almost "unfair" performance boost compared to others. If the bypass pty is disabled, `tpp` is as fast as the `ConPTY` lets it be, i.e. roughly on par with the windows console. 

The memory fooprint is interesting, `tpp` is the smallest after the `wsl` console, which is super extra small. 

Benchmark               | Terminal        | Memory (MB) | Time (s)
------------------------|-----------------|-------------|---------
alt-screen-random-write | alacritty       | 72.6        | 26.69
alt-screen-random-write | tpp             | 23.8        | 40.9 
alt-screen-random-write | tpp (bypass)    | 22.3        | **1.1**
alt-screen-random-write | wsl console     | **7.5**     | 39.15
alt-screen-random-write | win term        | 26.5        | 110.24
scrolling               | alacritty       | 78.2        | 1125
scrolling               | tpp             | 23.8        | 1209.49
scrolling               | tpp (bypass)    | 22.3        | **11.8**
scrolling               | wsl console     | **4.7**     | 876.08
scrolling               | win term        |             | 
scrolling-in-region     | alacritty       |             | 
scrolling-in-region     | tpp             |             |
scrolling-in-region     | tpp (bypass)    | 22.3        | **11.6**
scrolling-in-region     | wsl console     |             |
scrolling-in-region     | win term        |             |

> Windows 10 Home 1903, i7-8550u, 16GB RAM, NVMe disk. `alacritty` version 0.3.3, win term version 

## Linux

Even on linux where simple rendering using `xft` is used, `tpp` is surprisingly almost twice as fast as `alacritty` in the randrom write benchmark. It is 2nd after `alacritty` in the two scrolling benchmarks. 

Benchmark               | Terminal        | Memory (MB) | Time (s)
------------------------|-----------------|-------------|---------
alt-screen-random-write | alacritty       | 718         | 4.69
alt-screen-random-write | tpp             | 165         | **2.54**
alt-screen-random-write | sakura (libvte) | 569         | 17.32
alt-screen-random-write | xterm           | 92          | 41.84
alt-screen-random-write | st              | **73**      | 10.45
scrolling               | alacritty       | 718         | **4.3**
scrolling               | tpp             | 165         | 7.18
scrolling               | sakura (libvte) | 569         | 9.65
scrolling               | xterm           | 102         | 771
scrolling               | st              | **73**      | 22.89
scrolling-in-region     | alacritty       | 718         | **4.3**
scrolling-in-region     | tpp             | 165         | 7.11
scrolling-in-region     | sakura (libvte) | 569         | 10.18
scrolling-in-region     | xterm           | 102         | 792.31
scrolling-in-region     | st              | **73**      | 22.17

> lubuntu 18.04 LTS, i5-4200u, 4GB RAM, SSD M2 disk. `alacritty` version 0.3.3, `st` 0.8.2, `sakura` 3.5.0, `xterm` 330-lubuntu2.


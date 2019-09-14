# Terminal++ {#mainpage}

[![Windows Build](https://img.shields.io/azure-devops/build/zduka/2f98ce80-ca6f-4b09-aaeb-d40acaf97702/1?label=windows&logo=azure-pipelines)](https://dev.azure.com/zduka/tpp/_build?definitionId=1)
[![Linux Build](https://img.shields.io/azure-devops/build/zduka/2f98ce80-ca6f-4b09-aaeb-d40acaf97702/2?label=linux&logo=azure-pipelines)](https://dev.azure.com/zduka/tpp/_build?definitionId=2)
[![MacOS Build](https://img.shields.io/azure-devops/build/zduka/2f98ce80-ca6f-4b09-aaeb-d40acaf97702/3?label=macos&logo=azure-pipelines)](https://dev.azure.com/zduka/tpp/_build?definitionId=3)
[![Codacy Badge](https://api.codacy.com/project/badge/Grade/587d39b5db9e4742bb6ce05f86863439)](https://www.codacy.com/app/zduka/tpp?utm_source=github.com&amp;utm_medium=referral&amp;utm_content=zduka/tpp&amp;utm_campaign=Badge_Grade)
[![Language grade: C/C++](https://img.shields.io/lgtm/grade/cpp/g/zduka/tpp.svg?logo=lgtm&logoWidth=18)](https://lgtm.com/projects/g/zduka/tpp/context:cpp)

*Terminal++* or `tpp` is a minimalist cross-platform terminal emulator that is capable of emulating much of the commonly used `xterm` supported control sequences. 

> Please note that `tpp` is in a very early alpha stage and there may (and will be) rough edges. That said, it has been used by a few people as their daily driver for nearly a month now with only minor issues. 

# Configuration

The terminal settings can either be specified in the settings JSON file (it is a pseudo JSON with comments for clarity). Some of these can then be overriden on the commandline for each particular instance. Keyboard shortcuts cannot be configured at the moment.

## Keybindings

These are default keybinding settings:

`Ctrl +` and `Ctrl -` to increase / decrease zoom. 
`Ctrl Shift V` to paste from clipboard.
`Alt Space` to toggle full-screen mode.

## Command-line options

To show a list of command-line options and their meanings, run the terminal from a console:

    tpp --help

## Fonts

`tpp` should work with any font the underlying renderer can work with, but has been tested extensively with `Iosevka` which was (for now) copied in the repo (see the `fonts/LICENSE.md` for the fonts license.

Iosevka is also the first font `tpp` will try to use. If not found, defaults to `Consolas` on Windows and system font on Linux. This can be changed at any time either on the commandline, or in the settings.json

# Platform Support

`tpp` is supported natively on Windows and Linux. It runs on Apple computers as well, although the support is experimental and an X server is required for `tpp` to work at all. 

## Windows

On Windows, Windows 10 1903 and above are supported. `tpp` works with default `cmd.exe` prompt, Powershell and most importantly the Windows Subsystem for Linux (WSL). Accelerated rendering via DirectWrite is used. An installer is available. Settings are stored in `%APP_DATA%\\local\\terminalpp\\settings.json`.

To force `tpp` to use `cmd.exe` use the following:

    tpp --use-conpty -e cmd.exe

To force powershell, run:

    tpp --use-conpty -e powershell.exe

To force WSL with ConPTY, use:

    tpp --use-conpty -e wsl.exe

And finally, to force WSL with the bypass, use (assuming bypass is enabled in settings.json):

    tpp -e wsl.exe tpp-bypass

### ConPTY Bypass

If `tpp` is used with WSL, the [`tpp-bypass`](https://github.com/zduka/tpp-bypass) app can be used inside WSL to bypass the ConPTY terminal driver in Windows. This has numerous benefits such as full support for all escape sequences (i.e. mouse works) and speed (with the bypass enabled, `tpp` is the fastest terminal emulator on windows*). 

If the bypass is installed *before* `tpp`, `tpp` will automatically use it, otherwise it must be confifured manually. It is therefore recommended to install bypass before `tpp`. For installation, follow the instructions on the bypass' [website](https://github.com/zduka/tpp-bypass).

### Building from sources

To build under Windows, [Visual Studio](https://visualstudio.microsoft.com) and [cmake](https://cmake.org) must be installed. Open the Visual Studio Developer's prompt and get to the project directory. Then type the following commands:

    mkdir build
    cd build
    cmake ..
    cmake --build . --config Release

Then type the following to start the terminal:

    \tpp\Release\tpp.exe

### Running `tpp` in WSL

This is entirely possible if X server is installed on Windows, but should not be necessary. If you really need to, follow the linux installation guide.

## Linux

Although tested only on Ubuntu (native and WSL), all linux distributions should work just fine since `tpp` has minimal dependencies. On Linux, the configuration file is under `~/.config/tpp/settings.json` and `X11` with `xft` are used for the rendering. This can be pretty fast still due to `render` X server extension. 

At the moment, no installation packages are provided, but installing from sources is very simple:

### Building from sources

> On ubuntu systems, you can install all prerequisites by executing:

    sudo apt install cmake libx11-dev libxft-dev

Create the build directory and run `cmake`:

    mkdir build
    cmake .. -DCMAKE_BUILD_TYPE=Release
    make -j8

You can now execute the terminal by typing in the `build` directory:

    tpp/tpp

## Apple

When X server is installed, follow the Linux guide above. 

> Note that unlike Windows and Linux, Apple version is almost untested.

# Performance (*)

> Take the following section with a *big* grain of salt since the benchmark used is hardly representative or proper terminal workload. All it shows is peak performance of some terminal parts under very atrificial conditions. We are only using it because alacritty, which claims to be the fastest terminal out there uses it. 

Although raw peformance is not the main design goal of `tpp`, it is reasonably performant under both Windows and Linux. The terminal framerate is limited (configurable, but defaults to 60 FPS).

For lack of proper benchmarks, the [`vtebench`](https://github.com/jwilm/vtebench) from `alacritty` was used to test the basic performance. The `alt-screen-random-write`, `scrolling` and `scrolling-in-region` benchmarks are used because `tpp` does not support wide unicode characters yet. 

Informally, `tpp` with bypass trashes everyone else on Windows being at least 25x faster than the second best terminal (Alacritty). Note that the bypass is the key here since it seems that for many other terminals ConPTY is the bottleneck. 

On Linux, `tpp` comparable to `alacritty` and they swap places as the fastest terminal depending on the benchmark used. 

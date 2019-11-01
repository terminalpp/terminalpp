# Terminal++

*Terminal++* or `tpp` is a minimalist cross-platform terminal emulator that is capable of emulating much of the commonly used `xterm` supported control sequences. 

# Configuration

The terminal settings can be specified in the settings JSON file (it is a pseudo JSON with comments for clarity). Some of these can then be overriden on the commandline for each particular instance. Keyboard shortcuts cannot be configured at the moment.

The settings are stored under the following paths on different platforms:

- Windows: `%APP_DATA%\\Local\\terminalpp\\settings.json`
- Linux: `~/.config/terminalpp/settings.json`

Upon first execution of `tpp`, default settings will be created, which can later be edited by hand. Use `Ctrl+F10` in the terminal to open the settings file in default `JSON` editor. 

## Keybindings

These are default keybinding settings:

`Ctrl +` and `Ctrl -` to increase / decrease zoom. 
`Ctrl Shift V` to paste from clipboard.
`Alt Space` to toggle full-screen mode.
`Ctrl+F1` displays the about box with version information

## Command-line options

To show a list of command-line options and their meanings, run the terminal from a console:

    tpp --help

# Platform Specific Notes

`tpp` is supported natively on Windows and Linux. It *should* run on Apple computers as well, although the support is experimental and an X server is required for `tpp` to work at all. 

## Windows

On Windows, Windows 10 1903 and above are supported. `tpp` works with default `cmd.exe` prompt, Powershell and most importantly the Windows Subsystem for Linux (WSL). Accelerated rendering via DirectWrite is used.

### ConPTY Bypass

If `tpp` is used with WSL, the [`bypass`](https://github.com/terminalpp/bypass) app can be used inside WSL to bypass the ConPTY terminal driver in Windows. This has numerous benefits such as full support for all escape sequences (i.e. mouse works) and speed (with the bypass enabled, `tpp` is the fastest terminal emulator on windows*). 

If `wsl` is detected, `tpp` will attempt to install the bypass upon its first execution by downloading the binary appropriate for given `wsl` distribution. 

> Alternatively, the bypass can be downloaded from the repository above, built and installed manually as `~/.local/bin/tpp-bypass` where `tpp` will find it. Note that bypass check is only performed on first execution (i.e. when no settings.json exists).

### Running `tpp` in WSL

This is entirely possible if X server is installed on Windows, but should not be necessary. If you really need to, follow the linux installation guide.

## Linux

Although tested only on Ubuntu and OpenSUSE Leap (native and WSL), all linux distributions should work just fine since `tpp` has minimal dependencies. On Linux, `X11` with `xft` are used for the rendering. This can be pretty fast still due to `render` X server extension. 

## Apple

When X server is installed, follow the Linux guide above. 

> Note that unlike Windows and Linux, Apple version is untested. Also, since std::filesystem is available only since 10.15 and there are no 10.15 workers on azure. Not even build on OSX is tested at the moment.

# Building From Sources

Refer to the top-level `README` file for information on how to build `terminal++` from sources.
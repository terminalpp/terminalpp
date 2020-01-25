# Changelog

Lists the most important improvements for recent `terminalpp` versions.

### 0.5.2

- cursor appearance can be specified in `settings.json`
- scrolling position does not change when window focused out (issue #4)

### 0.5 - Remote File Opening

Adds basic support for remotely opening files (see the tpp-ropen repo for more details). Other minor improvements:

- session palette can be customized in settings.json
- terminal wait on terminated PTY is customized in settings.json
- bold text can be forced to render in bright colors
- snap store at edge
- Ubuntu WSL w/o version in name supported for bypass installation

(also, new repositories were created under the terminalpp organization).

### 0.4 - Terminal Scrolling

Terminal history added and turned on by default. The history is scrollable (wheel) and selectable. History is only stored for the normal buffer and is not even shown when alternate buffer is on. Other improvements:

- `Alt+F1` to show about box
- `Alt+F10` to open settings.json in default editor
- selection works when mouse is dragged outside the window as well (autoscrolls terminal if applicable)
- few bugfixes and code improvements

> Note that the release is alpha and updating from lower versions is very limited. A clean uninstall of the old version is recommended before installing v0.4.

### 0.3 - Font fallback

- font fallback, CJK, double width & height fonts

---
title: Changelog
layout: titled
---

Lists the most important improvements for recent `terminalpp` versions.

### v0.6 - Cummulative

This is a cummulative release before switching to the simpler UI framework currently in development. 

- simple tests framework in helpers, tests target added
- reentrant lock in helpers
- simpler events (single handler, std::function, method and fptr handlers)
- configuration code refactoring
- simpler UI code
- more automation for releases
- build fixed so that stamp is only generated when required
- better errors for invalid JSON settings
- paste confirmation dialog
- numeric keypad enter works (#6)

### 0.5.4

- macOS supported via QT
- paste confirmation 
- fixed windows default installation w/o WSL bug

### 0.5.3

- fixed bug with invalid JSON settings for initial installation

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

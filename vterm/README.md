# Virtual Terminal

The virtual terminal (`vterm`) is a class encapsulating the state of a terminal device.

## Renderer

The `vterm` is supposed to be paired with a renderer, which is a class responsible for rendering the contents of the terminal on a device. A simple renderer might be a GUI application which displays the contents of the terminal. 

## Connector

- a UI connector refreshes the terminal state via some UI classes it holds w/o the use of PTY at all
- a VT100/t++ connector should be extended by platform (Win32 ConPTY, *NIX, etc.)
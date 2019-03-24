# Virtual Terminal

The virtual terminal (`vterm`) is a class which describes the input and output interfaces of a terminal and implements its state.

# CHANGES

The renderer stays, but connector and virtual terminal will be merged together, i.e. each virtual terminal is its own connector, so when launching say a command, a ConPTYTppTerminal will be crated, which allows for VT100 and t++ senquences. 

^- this is simpler and better. Technically, the connector can still exist as a thing that just fills data to the terminal. *ACTUALLY IT SHOULD* !!


## Renderer

The `vterm` is supposed to be paired with a renderer, which is a class responsible for rendering the contents of the terminal on a device. A simple renderer might be a GUI application which displays the contents of the terminal. 

## Connector

- a UI connector refreshes the terminal state via some UI classes it holds w/o the use of PTY at all
- a VT100/t++ connector should be extended by platform (Win32 ConPTY, *NIX, etc.)
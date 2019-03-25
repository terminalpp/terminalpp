# Virtual Terminal

The virtual terminal (`vterm`) is a class which describes the input and output interfaces of a terminal and implements its state.

!! Single UI thread is assumed, or at least single ui thread per renderer-terminal-process etc chain. If not many things will break

# CHANGES

The renderer stays, but connector and virtual terminal will be merged together, i.e. each virtual terminal is its own connector, so when launching say a command, a ConPTYTppTerminal will be crated, which allows for VT100 and t++ senquences. 

^- this is simpler and better. Technically, the connector can still exist as a thing that just fills data to the terminal. *ACTUALLY IT SHOULD* !!

# Architecture

## `VTerm`

The virtual terminal stores the contents of the screen. 

## `Renderer`

## `Protocol`

## `Process`

- a UI connector refreshes the terminal state via some UI classes it holds w/o the use of PTY at all
- a VT100/t++ connector should be extended by platform (Win32 ConPTY, *NIX, etc.)
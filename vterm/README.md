# Virtual Terminal

The virtual terminal (`vterm`) is a class which describes the input and output interfaces of a terminal and implements its state.

!! Single UI thread is assumed, or at least single ui thread per renderer-terminal-process etc chain. If not many things will break

# Class Description

## `Terminal`

The base class for the virtual terminal, which implements storage of the terminal screen and defines the basic terminal events & structures. 

## `Renderer`

Renderer provides minimal interface and some common implementation useful for any class whose task is to display the contents of a `Terminal` on various devices, such as screen (the default expected from virtual terminals), or as part of an UI framework, or even encode the terminal's contents using ANSI escape codes and send it to other machine/ terminal. 

## `PTYTerminal`

The `PTYTerminal` adds a standardized way of communication between the terminal and its feeder (usually process) using streams. The exact nature of the streams is however not defined in the class since multiple options, such as network streams, OS PTY, etc. can be used. 

> `PTYTerminal` defines two interfaces - one for processing the input and displaying it correctly and the other one for implementing buffered reads and writes into the terminal streams. Children of `PTYTerminal` are expected to use virtual inheritance and define either various forms of decoding / encoding the events, or implement the stream accesses. 

## `VT100` 

Inherits (virtually) from `PTYTerminal` and provides decoding and encoding functions for the terminal input and events using the VT100 and parts of ANSI terminal sequences. 

## `ConPTYTerminal`

Available only on Windows, virtually inherits from `PTYTerminal` and describes the actual stream communication between the terminal and its attached process using Win32 ConPTY. 
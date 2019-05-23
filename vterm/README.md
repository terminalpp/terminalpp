# Virtual Terminal

The virtual terminal (`vterm`) is a class which describes the input and output interfaces of a terminal and implements its state.

!! Single UI thread is assumed, or at least single ui thread per renderer-terminal-process etc chain. If not many things will break

# Overview

The `Terminal` class encapsulates the screen buffer (drawable cells) and state (cursor, etc.) and defines a simple API capable of things like:

- resizing the terminal
- sending key & mouse events
- sending clipboard events

The `Terminal::Renderer` class sits between the `Terminal` and the user. It connects to terminal via events, renders its contents and sends to it the user input events.

The `Terminal::Backend` class is paired with particular terminal and it connects to the terminal client responsible for reacting on the terminal user events and filling in the terminal buffer. For most scenarios, the backend decodes whatever communication protocol the client uses into terminal buffer updates and similarly encodes the user input events and sends them to the client. 

The terminal backend only provides API for interfacing the terminal itself, it leaves the means of connecting to the client process on its children. One of them is the `Terminal::PTYBackend` which specifies a primitive pseudoterminal-like API for connecting to pseudoterminals using input and output streams (these should inherit from the `PTY` class).

Finally, the `LocalPTY` inherits from `PTY` and defines for each of the supported operating systems a pseudoterminal for locally running processes. This uses pseudoterminals in Linux and ConPTY on Windows. 

# The VT100 Backend

The `VT100` class inherits from `Terminal::PTYBackend` and implements much of the `xterm` & VT100 ansi codes. Detailed information about the escape sequences `VT100` backend understands can be found in a separate file.   

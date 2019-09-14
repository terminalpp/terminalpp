# ui - Terminal User Interface Framework

A simple library which implements a text user interface capable of rendering in a terminal, supporting both mouse and keyboard inputs. 

## Mouse Events

Mouse events are always dispatched to the widget underneath the mouse cursor. 

## Keyboard Events & Cursor

Keyboard events are dispatched to widget which has the keyboard focus. The widget with focus can also supply information about the cursor and where to render it. 


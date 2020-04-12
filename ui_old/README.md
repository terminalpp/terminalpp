# ui - Terminal User Interface Framework

A simple library which implements a text user interface capable of rendering in a terminal, supporting both mouse and keyboard inputs. 

## Overview

The ui toolkit consists of a few classes providing the basic functionality and implementations of several ui controls built on top of these. The basic classes, all of which are described later in greater detail, are:

- `ui::Widget` - the base class for all visual components. Implements basic properties (visibility, position, size, etc.), events and methods (repainting, etc. ) common to every widget in the framework.
- `ui::Canvas` - the canvas on which the widget draws its contents when repainted. A visible part of a canvas is backed by a buffer which actually holds the information for each cell.
- `ui::Container` - while each widget can be a parent widget to others, the Container also keeps track of its own children so that they can be added and removed at runtime. Also defines the layout of the children so that they can be repositional automatically and react on the container size or position change. 
- `ui::RootWindow` - a widget that wraps around a canvas buffer for the entire screen. Root window anso interface with renderers to define inform way of displaying the window's content to the user and relays the user events such as clipboard, mouse and keyboard. 
- `ui::Renderer` determines the interface a rendering component of a root window must support

## Base Widget Implementation - `ui::widget`


### Keyboard Focus

The `ui::RootWindow` maintains the keyboard focus for the particular renderer. At any given time, only one child widget of the root window can have keyboard focus. To explicitly set keyboard focus on a widget, call its `ui::Widget::setFocused()` method. 

Only widgets which are enabled can be focused. When a widget is disabled, it looses focus automatically. 

Keyboard focus can also be managed explicitly via `ui::Widget::focusStop()` and `ui::Widget::focusIndex()`. The `focusIndex` property determines the index of the element in the automatic focus group. No two elements in same group (root window) can have same index. If a widget is also a `focusStop`, then root window can automatically switch between these via the `ui::RootWindow::focusNext()`.

## Mouse Events

Mouse events are always dispatched to the widget underneath the mouse cursor. 

## Keyboard Events & Cursor

Keyboard events are dispatched to widget which has the keyboard focus. The widget with focus can also supply information about the cursor and where to render it. 


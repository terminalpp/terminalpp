# Text UI Framework v2

Buffer is holds the screen contents. 

Renderer is responsible for rendering the buffer and for the user interaction, namely:

- mouse
- keyboard
- resizing
- clipboard

Widget is a control that can interact with a user and draw itself on a buffer

- widget is passive and is not aware of any renderers, it has the OnRepaint() event which the renderer should subscribe to. 
- widget are aware of their parent widget, 


# Threading & Synchronization

All modifications to the UI must be done b
#pragma once

#include "ui/widget.h"

namespace ui {

    /** Base terminal specification. 
     
        Provides minimal interface of a terminal. On top of being a widget, provides the events for adding new history line and changing the active buffer. 
     */
    class Terminal : public virtual Widget {
    public:
        using Cell = Canvas::Cell;
        using Cursor = Canvas::Cursor;

        enum class BufferKind {
            Normal,
            Alternate,
        };

        using BufferChangeEvent = Event<BufferKind>;

        struct BufferRow {
        public:
            BufferKind buffer;
            int width;
            Cell const * cells;
        };

        using NewHistoryRowEvent = Event<BufferRow>;

        /** Triggered when the terminal enables, or disables an alternate mode. 
         
            Can be called from any thread, expects the terminal buffer lock.
         */
        BufferChangeEvent onBufferChange;

        /** Triggered when new row is evicted from the terminal's scroll region. 
         
            Can be triggered from any thread, expects the terminal buffer lock. The cells will be reused by the terminal after the call returns. 
         */
        NewHistoryRowEvent onNewHistoryRow;

    }; // ui::Terminal



} // namespace ui
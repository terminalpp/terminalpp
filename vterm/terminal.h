#pragma once
#include <type_traits>
#include <thread>
#include <deque>

#include "ui/canvas.h"
#include "ui/widget.h"
#include "ui/selection.h"
#include "ui/builders.h"
#include "ui/widgets/scrollbox.h"

#include "pty.h"

namespace vterm {

    typedef helpers::EventPayload<helpers::ExitCode, ui::Widget> ExitCodeEvent;

    typedef helpers::EventPayload<unsigned, ui::Widget> LineScrollEvent;

    /** The input buffer of the terminal and its length. 
     
        TODO this looks very generic and perhaps can exist in some generic place too.
     */
    struct InputBuffer {
        char const * buffer;
        size_t size;
    };

    typedef helpers::EventPayload<InputBuffer, ui::Widget> InputProcessedEvent;

    class Terminal : public ui::ScrollBox, public ui::SelectionOwner {
    public:

        typedef ui::Cell Cell;
        typedef ui::Cursor Cursor;

        using Widget::setFocused;
        using Widget::setFocusStop;
        using Widget::setFocusIndex;
        using Widget::setEnabled;

        /** The terminal buffer holds information about the cells and cursor. 

        */
        class Buffer {
        public:

            typedef helpers::SmartRAIIPtr<Buffer> Ptr;

            Buffer(int cols, int rows);

            Buffer(Buffer const & from);

            Buffer & operator = (Buffer const & other);

            ~Buffer();

            int cols() const {
                return cols_;
            }

            int rows() const {
                return rows_;
            }

            Cell const & at(int x, int y) const {
                ASSERT(x >= 0 && x <= cols_ && y >= 0 && y < rows_);
                return cells_[y][x];
            }

            Cell & at(int x, int y) {
                ASSERT(x >= 0 && x <= cols_ && y >= 0 && y < rows_);
                return cells_[y][x];
            }

            Cell const & at(ui::Point pos) const {
                return at(pos.x, pos.y);
            }

            Cell & at(ui::Point pos) {
                return at(pos.x, pos.y);
            }

            Cursor const & cursor() const {
                return cursor_;
            }

            Cursor & cursor() {
                return cursor_;
            }

            void lock() {
                lock_.lock();
            }

            void priorityLock() {
                lock_.priorityLock();
            }

            void unlock() {
                lock_.unlock();
            }

            void resize(int cols, int rows);

            /** Inserts given number of lines at given top row.
                
                Scrolls down all lines between top and bottom accordingly. Fills the new lines with the provided cell.
                */
            void insertLines(int lines, int top, int bottom, Cell const & fill);

            /** Deletes given number of lines at given top row.
                
                Scrolls up all lines between top and bottom accordingly. Fills the new lines at the bottom with the provided cell.
                */
            void deleteLines(int lines, int top, int bottom, Cell const & fill);

            /** Returns the given line as a string.
             
                TODO This should preserve the attributes somehow.
             */
            std::string getLine(int line);

        private:

            /** Fills the specified row with given character. 

                Copies larger and larger number of cells at once to be more efficient than simple linear copy. 
                */
            void fillRow(Cell* row, Cell const& fill, unsigned cols);

            /** Resizes the cell buffer. 

                First creates the new buffer and clears it. Then we determine the latest line to be copied (since the client app is supposed to rewrite the current line). We also calculate the offset of this line to the current cursor line, which is important if the last line spans multiple terminal lines since we must adjust the cursor position accordingly then. 

                The line is the copied. 
            */
            void resizeCells(int newCols, int newRows);


            /* Size of the buffer and the array of the cells. */
            int cols_;
            int rows_;
            Cell ** cells_;

            Cursor cursor_;

            /* Priority lock on the buffer. */
            helpers::PriorityLock lock_;


        }; // Terminal::Buffer 

        // events

        /** Triggered when the attached pseudoterminal terminates. 
         
            Note that when the terminal is deleted, its PTY is forcibly terminated at first, so the event will fire precisely once in the terminal lifetime. 
         */
        helpers::Event<ExitCodeEvent> onPTYTerminated;

        helpers::Event<ui::StringEvent> onTitleChange;

        helpers::Event<ui::VoidEvent> onNotification;

        /** Triggered when a terminal line has been scrolled out. 
         
            It is assumed that the topmost line is always the line scrolled out. 
         */
        helpers::Event<LineScrollEvent> onLineScrolledOut;

        /** Triggered when new input has been processed by the terminal. 
         */
        helpers::Event<InputProcessedEvent> onInput;

        // methods

        PTY * pty() {
            return pty_;
        }

        /** Returns the title of the terminal. 
         */
        std::string const & title() const {
            return title_;
        }

        /** Deletes the terminal and the attached PTY. 
         
            Waits for the reading and process exit terminating threads to finalize. 
         */
        ~Terminal() {
            fps_ = 0;
            delete pty_;
            ptyReader_.join();
            ptyListener_.join();
            repainter_.join();
        }

        ui::Point scrollOffset() const override {
            return ui::ScrollBox::scrollOffset();
        }

        ui::Rect clientRect() const override {
            return ui::ScrollBox::clientRect();
        }

    protected:

        /** Creates the terminal with the given PTY. 

            Starts the process exit monitoring thread and the receiving thread to read input from the pty. 
         */
        Terminal(int width, int height, PTY * pty, unsigned fps, size_t ptyBufferSize = 10240);

        /** Returns the locked buffer. 
         */
        Buffer::Ptr buffer(bool priority = false) {
            if (priority)
                buffer_.priorityLock();
            else
                buffer_.lock();
            return Buffer::Ptr(buffer_, false);
        }

        /** Copies the contents of the terminal's buffer to the ui canvas. 
         */
        void paint(ui::Canvas & canvas) override;

        /** Updates the FPS ratio of the terminal. 
         
            Note that the FPS is not precise since triggering the repaint event itself is not accounted for in the wait. This should not be a problem since the terminal is not really hard real-time affair.
         */
        void setFPS(unsigned value) {
            ASSERT(value != 0);
            fps_ = value;
        }

        void setTitle(std::string const & value) {
            if (title_ != value) {
                title_ = value;
                updateTitle(value);
            }
        }

        void send(char const * buffer, size_t size) {
            pty_->send(buffer, size);
            // TODO check errors
        }

        void send(std::string const & str) {
            send(str.c_str(), str.size());
        }

        /** When terminal size is updated, we must resize the underlying buffer and the PTY.
         */
        void updateSize(int width, int height) override {
            {
                Buffer::Ptr b = buffer(true); // grab priority lock
                b->resize(width, height);
            }
            pty_->resize(width, height);
            // resize the client canvas
            setClientArea(width, buffer_.rows() + static_cast<int>(history_.size())); 
            ui::ScrollBox::updateSize(width, height);
            ui::Widget::updateSize(width, height);
        }

        void updateFocused(bool value) override {
            Widget::updateFocused(value);
            repaint();
        }

        // mouse events to deal with the selection

        void mouseDown(int col, int row, ui::MouseButton button, ui::Key modifiers) override;
        void mouseUp(int col, int row, ui::MouseButton button, ui::Key modifiers) override;

        void mouseWheel(int col, int row, int by, ui::Key modifiers) override {
            MARK_AS_UNUSED(col);
            MARK_AS_UNUSED(row);
            MARK_AS_UNUSED(modifiers);
            if (scrollable_ && ! history_.empty()) {
                scrollVertical(-by);
                repaint();
            } 
            ui::Widget::mouseWheel(col, row, by, modifiers);
        }

        void mouseMove(int col, int row, ui::Key modifiers) override;

        void mouseOut() override {
            ui::ScrollBox::mouseOut();
        }


        /** When selection is invalidated, we request repaint so that the selection is no longer displayed. 
         */
        /*void invalidateSelection() override {
            Clipboard::invalidateSelection();
            repaint();
        } */

        /** Returns the contents of the selection. 
         
            Trims the right of each line.
         */
        std::string getSelectionContents() override;

        void selectionUpdated() override {
            repaint();
        }

        void repaint() override {
            repaint_ = true;
        }

        // terminal interface


        /** Called by the terminal when new input data is available from the pty. 
         
            The method should obtain locked buffer and update it according to the received data. 
         */
        virtual size_t processInput(char * buffer, size_t bufferSize) = 0;

        /** Called when the attached PTY has terminated. 
         */
        virtual void ptyTerminated(helpers::ExitCode exitCode) {
            trigger(onPTYTerminated, exitCode);
        }

        virtual void updateTitle(std::string const & title) {
            trigger(onTitleChange, title);
        }

        virtual void notify() {
            trigger(onNotification);
        }

        void enableScrolling(bool value) {
            if (scrollable_ != value) {
                scrollable_ = value;
            }
        }

        virtual void lineScrolledOut(int lines) {
            if (! scrollable_)
                return;
            if (historySizeLimit_ > 0) {
                for (int i = 0; i < lines; ++i) {
                    std::string line = buffer_.getLine(i);

                    history_.push_back(line);
                    if (history_.size() > historySizeLimit_) {
                        history_.pop_front();
                    } else {
                        setClientArea(width(), buffer_.rows() + static_cast<int>(history_.size())); 
                        // TODO only scroll if we were at the top
                        setScrollOffset(scrollOffset() + ui::Point{0, 1});
                    }
                }
            }

            if (onLineScrolledOut.attachedHandlers() > 0) {
                buffer_.unlock();
                trigger(onLineScrolledOut, lines);
                buffer_.lock();
            }
        }

        /* Cells and cursor. */
        Buffer buffer_;

    private:
    
        /* Pseudoterminal for communication */
        PTY * pty_;

        /* Thread which reads data from the attached pseudoterminal. */
        std::thread ptyReader_;

        /* Thread which waits for the attached pty to be terminated. */
        std::thread ptyListener_;

        /* The requested number of redraws */
        volatile unsigned fps_;
        std::thread repainter_;
        std::atomic<bool> repaint_;

        /* Terminal title */
        std::string title_;

        /** Determines whether mouse selection update is in progress or not. 
         */
        bool mouseSelectionUpdate_;

        /** Determines whethet the terminal is scrollable, or not. 
         */
        bool scrollable_;

        size_t historySizeLimit_;

        std::deque<std::string> history_;

    }; // vterm::Terminal


    EVENT_BUILDER(OnTitleChange, ui::StringEvent, onTitleChange, Terminal);
    EVENT_BUILDER(OnNotification, ui::VoidEvent, onNotification, Terminal);
    EVENT_BUILDER(OnPTYTerminated, ExitCodeEvent, onPTYTerminated, Terminal);
    EVENT_BUILDER(OnLineScrolledOut, LineScrollEvent, onLineScrolledOut, Terminal);
    EVENT_BUILDER(OnInput, InputProcessedEvent, onInput, Terminal);

} // namespace vterm

namespace ui {

    template<>
    inline void ui::Canvas::copyBuffer<vterm::Terminal::Buffer>(int x, int y, vterm::Terminal::Buffer const & buffer) {
        int xe = std::min(x + buffer.cols(), width()) - x;
        int ye = std::min(y + buffer.rows(), height()) - y;
        for (int by = 0; by < ye; ++by)
            for (int bx = 0; bx < xe; ++bx)
                if (Cell * c = at(Point(x + bx, y + by))) 
                    *c = buffer.at(bx, by);
    }

}
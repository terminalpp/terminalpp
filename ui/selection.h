#pragma once

#include "shapes.h"
#include "widget.h"

/** @page uiselection ui - Selection & Clipboard

    How to do selection on scrollbox? 

    Ideally I would have widget to return the scroll properties, but actually not keep them, i.e. it would always return 0,0, width, height. 

    Then scrollbox would actually change this - since vcall is cheap. 

 */

namespace ui {

    /** Determines selection coordinates on a widget. 

        The selection is inclusive of start, but exclusive of the end cell in both column and row. 

        TODO for now, this is a simple from - to point selection, but in the future, the selection can actually be rectangular, or so or so...
     */
    class Selection {
    public:

        /** Default constructor creates an empty selection. 
         */
        Selection():
            start_(Point(0, 0)),
            end_(Point(0, 0)) {
        }

        /** Creates a selection between two *inclusive* cells. 
         
            Reorders the cells if necessary. 
         */
        static Selection Create(Point start, Point end) {
            if (end.y < start.y) {
                std::swap(start, end);
                --end.x;
            } else if (end.y == start.y && end.x < start.x) {
                std::swap(start, end);
                --end.x;
            }
            // because the cells themselves are inclusive, but the selection is exclusive on its end, we have to increment the end cell
            end += Point(1,1);
            return Selection(start, end);
        }

        /** Clears the selection. 
         */
        void clear() {
            start_ = Point(0,0);
            end_ = Point(0, 0);
        }

        /** Returns true if the selection is empty. 
         */
        bool empty() const {
            return start_.y == end_.y;
        }

        /** Returns the first cell of the selection (inclusive). 
         */
        Point start() const {
            return start_;
        }

        /** Returns the last cell of the selection (exclusive).
         */
        Point end() const {
            return end_;
        }

    protected:
        Selection(Point const & start, Point const & end):
            start_(start),
            end_(end) {
        }

        Point start_;
        Point end_;

    }; // ui::Selection

    /** Provides basic API for selection & clipboard handling. 
     
        The renderer caches the clipboard and selection buffer contents and remembers the ownership of selection. 
     */
    class SelectionOwner : public virtual Widget {
    public:

        /** Returns the current selection region coordinates.
         */
        Selection const & selection() const {
            return selection_;
        }

        /** Returns the selection contents. 
         
            This method has to be overriden in children to provide the mechanism how to obtain the selection contents from given selection. 
         */
        virtual std::string getSelectionContents() = 0;

        /** Clears the selection held. 
         
            Clears the selection rectangle visualization and informs the root window that selection has been cleared, which in turn informs the renderer that selection ownership has been released. 
         */
        void clearSelection();


    protected:

        friend class RootWindow;

        SelectionOwner():
            updating_{false} {
        }

        /** Informs the root window that clipboard contents has changed to the current selection. 
         */
        void setClipboard();

        /** Informs the root window that the clipboard contents has changed to the given string regardless of the selection status. 
         */
        void setClipboard(std::string const & value);

        /** Informs the root window that selection should now be held by the current widget. 
         */
        void registerSelection();


        /** Called whenever the selection rectangle is updated so that it can be redrawn. 
         
            Calls the repaint() method by default, but can be overriden in children once the selection contents mechanism is known. 
         */
        virtual void selectionUpdated() {
            repaint();
        }

        /** Called when the selection should be invalidated. 
         */
        virtual void selectionInvalidated() {
            selection_.clear();
            selectionUpdated();
        }

        void startSelectionUpdate(int col, int row) {
            ASSERT(! updating_);
            if (!selection_.empty())
                clearSelection();
            updating_ = true;
            selectionStart_ = Point{col, row} + scrollOffset();
        }

        /** Updates the selection. 
         */
        void selectionUpdate(int col, int row) {
            if (! updating_)
                return;
            if (col < 0)
                col = 0;
            else if (col >= width())
                col = width() - 1;
            if (row < 0) 
                row = 0;
            else if (row >= height())
                row = height() - 1;
            // update the selection and call for repaint
            selection_ = Selection::Create(selectionStart_, Point{col, row} + scrollOffset());
            selectionUpdated();
        }

        void endSelectionUpdate(int col, int row) {
            MARK_AS_UNUSED(col);
            MARK_AS_UNUSED(row);
            if (! updating_)
                return;
            updating_ = false;
            registerSelection();
        }

        virtual void paintSelection(Canvas & canvas) {
            if (selection_.empty())
                return;
            ui::Brush selBrush(ui::Color(192, 192, 255, 128));
            canvas.fill(selection(), selBrush); 
        }

        /** Cancels selection update in progress. 
         
            If mouseOut even is reported in the middle of updating the selection, the selection is cancelled. Note that normally, mouse leave should not be received until the selection update is done as the mouse is supposed to be locked. 
         */
        void mouseOut() override {
            if (updating_) {
                updating_ = false;
                selection_.clear();
                selectionUpdated();
            }
        }

    private:
        Selection selection_;
        bool updating_;

        Point selectionStart_;

    };

} // namespace ui
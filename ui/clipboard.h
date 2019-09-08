#pragma once

#include "shapes.h"
#include "widget.h"

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
        bool empty() {
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


    /* Clipboard & selection manager for the widgets. 

       Provides necessary interface for widgets to deal with clipboard and selection events. 

       
     */
    class Clipboard : public virtual Widget {
    public:



    protected:
        friend class RootWindow;

        Clipboard():
            selectionStart_{-1,-1} {
        }

        /** Requests the paste of clipboard data. 
         */
        void requestClipboardPaste();

        /** Requests the paste of selection data. 
         */
        void requestSelectionPaste();

        void setClipboard(std::string const & contents);

        void setSelection(std::string const & contents);

        /** First invalidates the selection which allows the current widget to react to the change, then informs the root window that the selection was cleared. 
         
            Does nothing if the selection is empty already. 
         */
        void clearSelection();

        /** Called when the clipboard or selection contents becomes available. 
         */
        virtual void paste(std::string const & contents) = 0;

        /** Triggered if the selection should be cleared due to external forces. 
         */ 
        virtual void invalidateSelection() {
            selection_.clear();
        }

        void updateSelectionRegionStart(Point const & start) {
            selection_.clear();
            selectionStart_ = start;
        }

        void updateSelectionRegion(Point end) {
            if (end.x < 0)
                end.x = 0;
            else if (end.x >= width())
                end.x = width() - 1;
            if (end.y < 0)
                end.y = 0;
            else if (end.y >= height())
                end.y = height() - 1;
            selection_ = Selection::Create(selectionStart_, end);
        }

        void updateSelectionRegionStop() {
            selectionStart_ = Point{-1,-1};
        }

        bool updatingSelectionRegion() const {
            return selectionStart_.x >= 0;
        }

        /** The selection that is managed by the object. 
         */
        Selection selection_;

    private:
        Point selectionStart_;


    }; // ui::Clipboard

} // namespace ui
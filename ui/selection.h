#pragma once

#include "shapes.h"
#include "canvas.h"
#include "root_window.h"

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

    private:

        Selection(Point const & start, Point const & end):
            start_(start),
            end_(end) {
        }

        Point start_;
        Point end_;

    }; // ui::Selection

    /** Widget mixin for selection ownership and manipulation. 
     
        Each widget that wishes to support user selections should inherit from the this class as well. 

        > Note that simply being able to receive clipboard or selection contents is feature present in every widget using the Widget::paste() method. 

     */
    template<typename T>
    class SelectionOwner {
    public:

        SelectionOwner():
            selectionStart_{-1,-1} {
        }

        Selection const & selection() const {
            return selection_;
        }

        /** Returns the selection contents. 
         
            Should return empty string if the selection is empty or invalid. 
         */
        virtual std::string getSelectionContents() const = 0;

    protected:

        /** Returns true if the selection update is in progress. 
         */
        bool updatingSelection() const {
            return selectionStart_.x != -1;
        }

        /** Starts the selection update. 
         
            If the widget already has a non-empty selection, clears the selection first and then resets the selection process.
         */
        void startSelection(Point start) {
            if (! selection_.empty())
                clearSelection();
            if (! updatingSelection() && ! selection_.empty())
                clearSelection();
            selectionStart_ = start;
        }

        virtual void updateSelection(Point end, Point clientSize) {
            // don't do anything if we are not updating the selection
            if (! updatingSelection())
                return;
            end.x = std::max(0, end.x);
            end.x = std::min(clientSize.x - 1, end.x);
            end.y = std::max(0, end.y);
            end.y = std::min(clientSize.y - 1, end.y);
            // update the selection and call for repaint
            selection_ = Selection::Create(selectionStart_, end);
            static_cast<T*>(this)->repaint();
        }

        /** Finishes the selection update, obtains its contents and registers itself as the selection owner. 
         */
        void endSelection() {
            selectionStart_.x = -1;
            if (! selection_.empty()) {
                std::string contents{getSelectionContents()};
                RootWindow * rw = static_cast<T*>(this)->rootWindow();
                ASSERT(rw != nullptr);
                if (rw != nullptr) 
                    rw->setSelection(static_cast<T*>(this), contents);
            }
        }

        void cancelSelection() {
            if (updatingSelection()) {
                selectionStart_.x = -1;
                if (!selection_.empty()) {
                    selection_.clear();
                    static_cast<T*>(this)->repaint();
                }
            }
        }

        /** Notifies the root window that the selection should be cleared. 
         
            The root window then informs the widget and the window that the selection has been invalidated. 
         */
        void clearSelection() {
            RootWindow * rw = static_cast<T*>(this)->rootWindow();
            ASSERT(rw != nullptr);
            if (rw != nullptr) 
                rw->clearSelection();
        }

        /** Invalidates the selection and repaints the control. 
         */
        virtual void selectionInvalidated() {
            selection_.clear();
            selectionStart_.x = -1;
            static_cast<T*>(this)->repaint();
        }

        /** Marks the selection on the given canvas. 
         */
        virtual void paintSelection(Canvas & canvas, Color color) {
            if (selection_.empty())
                return;
            ui::Brush selBrush(color);
            canvas.fill(selection_, selBrush); 
        }

    private:
        Selection selection_;
        Point selectionStart_;
    }; // ui::SelectionOwner

} // namespace ui

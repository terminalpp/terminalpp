#pragma once

#include "../geometry.h"

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
            start_{Point{0, 0}},
            end_{Point{0, 0}} {
        }

        /** Creates a selection between two *inclusive* cells. 
         
            Reorders the cells if necessary. 
         */
        static Selection Create(Point start, Point end) {
            if (end.y() < start.y()) {
                std::swap(start, end);
                end.setX(end.x() - 1);
            } else if (end.y() == start.y() && end.x() < start.x()) {
                std::swap(start, end);
                end.setX(end.x() - 1);
            }
            // because the cells themselves are inclusive, but the selection is exclusive on its end, we have to increment the end cell
            end += Point{1,1};
            return Selection(start, end);
        }

        /** Clears the selection. 
         */
        void clear() {
            start_ = Point{0,0};
            end_ = Point{0, 0};
        }

        /** Returns true if the selection is empty. 
         */
        bool empty() const {
            return start_.y() == end_.y();
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
            start_{start},
            end_{end} {
        }

        Point start_;
        Point end_;

    }; // ui::Selection


    /** Widget mixin for selection ownership and manipulation. 
     */
    template<typename T>
    class SelectionOwner : public TraitBase<SelectionOwner, T> {
    public:
        SelectionOwner():
            selectionStart_{-1, -1} {
        }

    protected:
        using TraitBase<SelectionOwner, T>::downcastThis;

        /** Given the current selection, returns its contents. 
         */
        virtual std::string getSelectionContents() const = 0;

        Selection const & selection() const {
            return selection_;
        }
        /** Returns true if the selection update is in progress. 
         */

        bool updatingSelection() const {
            return selectionStart_.x() != -1;
        }

        /** Clears the selection and repaints the control. 
         */
        virtual void clearSelection() {
            selection_.clear();
            selectionStart_ = Point{-1, -1};
            downcastThis()->repaint();
        }
        
        /** Marks the selection on the given canvas. 
         */
        void paintSelection(Canvas & canvas, Color background) {
            if (selection_.empty())
                return;
            if (selection_.start().y() + 1 == selection_.end().y()) {
                canvas.fillRect(Rect::FromCorners(selection_.start(), selection_.end()), background);
            } else {
                canvas.fillRect(Rect::FromCorners(selection_.start(), Point{canvas.width(), selection_.start().y() + 1}), background);
                canvas.fillRect(Rect::FromTopLeftWH(0, selection_.start().y() + 1, canvas.width(), selection_.end().y() - selection_.start().y() - 2), background);
                canvas.fillRect(Rect::FromCorners(Point{0, selection_.end().y() - 1}, selection_.end()), background);
            }
        }

        /** \name Selection Update
         */
        //@{

        /** Starts the selection update. 
         
            If the widget already has a non-empty selection, clears the selection first and then resets the selection process.
         */
        void startSelectionUpdate(Point start) {
            if (! selection_.empty())
                clearSelection();
            selectionStart_ = start;
        }

        void updateSelection(Point end, Point clientSize) {
            // don't do anything if we are not updating the selection
            if (! updatingSelection())
                return;
            end.setX(std::max(0, end.x()));
            end.setX(std::min(clientSize.x() - 1, end.x()));
            end.setY(std::max(0, end.y()));
            end.setY(std::min(clientSize.y() - 1, end.y()));
            // update the selection and call for repaint
            selection_ = Selection::Create(selectionStart_, end);
            downcastThis()->repaint();
        }

        /** Finishes the selection update, obtains its contents and registers itself as the selection owner. 
         */
        void endSelectionUpdate() {
            selectionStart_ = Point{-1, -1};
            if (! selection_.empty()) {
                std::string contents{getSelectionContents()};
                downcastThis()->registerSelection(contents);
            }
        }

        void cancelSelectionUpdate() {
            if (updatingSelection()) {
                selectionStart_ = Point{-1, -1};
                if (!selection_.empty()) {
                    selection_.clear();
                    downcastThis()->repaint();
                }
            }
        }
        //@}

    private:
        Selection selection_;
        Point selectionStart_;
    }; 

} // namespace ui
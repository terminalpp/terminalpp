#pragma once

#include "shapes.h"

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

} // namespace ui
#pragma once

#include "selection.h"
#include "widget.h"

namespace ui {

    class Renderer;

    /* Clipboard & selection manager for the widgets. 

       Provides necessary interface for widgets to deal with clipboard and selection events. 

       
     */
    class Clipboard : public virtual Widget {
    public:



    protected:
        friend class Renderer;

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

        void updateSelectionRegion(Point const & end) {
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
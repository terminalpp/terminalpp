#pragma once

#include "../styled_widget.h"
#include "../layout.h"

namespace ui {

    class Pager : public StyledWidget {
    public:

        Pager() {
            setLayout(new Layout::Maximized{});
        }

        /** Adds new page to the pager and makes the page active, triggering the onPageChange event. 
         */
        void addPage(Widget * page) {
            attach(page);            
            Event<Widget*>::Payload p{page};
            onPageChange(p, this);
        }

        /** Removes given page from the pager. 
         
            If the removed page is active, moves the active page to the previous one and triggers the onPageChange event. 
         */
        void removePage(Widget * page) {
            Widget * oldActivePage = activePage();
            detach(page);
            if (page == oldActivePage) {
                Event<Widget*>::Payload p{activePage()};
                onPageChange(p, this);
            }
        }

        /** Returns the currently active page. 
         */
        Widget * activePage() const {
            if (children().empty())
                return nullptr;
            return children().back();
        }

        /** Sets the currently active page to given widget.
         
            The widget must already be a page (child) of the pager. 
         */
        void setActivePage(Widget * page) {
            ASSERT(page->parent() == this);
            // re-attaching puts the page in the visible page position
            if (page != activePage()) 
                addPage(page);
        }

        Event<Widget*> onPageChange;

    protected:

        void paint(Canvas & canvas) override {
            // paint the background first
            paintBackground(canvas);
            // and paint the active page, if any
            if (! children().empty())
                paintChild(children().back());
            // finally paint the border
            paintBorder(canvas);
        }

    }; // ui::Pager


} // namespace ui
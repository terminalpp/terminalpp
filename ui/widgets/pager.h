#pragma once

#include "../widget.h"
#include "../layout.h"

namespace ui {

    /**
     */
    class Pager : public Widget {
    public:

        Pager() {
            setLayout(new Layout::Maximized{});
        }

        /** Adds new page to the pager and makes the page active, triggering the onPageChange event. 
         */
        void addPage(Widget * page) {
            if (! children().empty())
                children().back()->setVisible(false);
            attach(page);
            page->setVisible(true);
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

        /** Only draws the active page as all other are supposed to be invisible. 
         */
        void paint(Canvas & canvas) override {
            MARK_AS_UNUSED(canvas);
            // paint the active page, if any
            if (! children().empty())
                paintChild(children().back());
        }

    }; // ui::Pager


} // namespace ui
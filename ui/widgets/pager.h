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

        /** Sets the currently active page to given widget.
         
            If the given page is not yet pager's child, it is added as child as well. 
         */
        void setActivePage(Widget * page) {
            // re-attaching puts the page in the visible page position
            if (page != activePage()) {
                if (! children().empty())
                    children().back()->setVisible(false);
                attach(page);
                page->setVisible(true);
                Event<Widget*>::Payload p{page};
                onPageChange(p, this);

            }
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
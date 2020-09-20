#pragma once

#include "../widget.h"
#include "modal_pane.h"
#include "dialog.h"

namespace ui {

    /** Basic root widget. 
     */
    class Window : public virtual Widget {
    public:

        Widget const * contents() const {
            return contents_;
        }

        Widget * contents() {
            return contents_;
        }

    protected:
       
        Window():
            modalPane_{new ModalPane{}} {
            setLayout(new Layout::Maximized{});
            attach(modalPane_);
        }

        /** Sets the contents of the window. 
         
            Note that the old contents, if any, is *not* deleted by this method. 

         */
        virtual void setContents(Widget * value) {
            if (contents_ != value) {
                if (contents_ != nullptr)
                    detach(contents_);
                contents_ = value;
                // because the modal pane is always over the contents
                attachBack(contents_);
            }
        }

        /** Shows the given widget modally. 
         */
        void showModal(Widget * w) {
            modalPane_->attach(w);
        }

        /** Shows an error box. 
         */
        void showError(std::string const & error) {
            schedule([this, error]() {
                Dialog::Error * e = new Dialog::Error{error};
                showModal(e);
            });
        }


    private:

        ModalPane * modalPane_;
        Widget * contents_ = nullptr;

    }; // ui::Window

} // namespace ui
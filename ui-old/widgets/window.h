#pragma once

#include "panel.h"
#include "dialog.h"

namespace ui {

    /** Simple window. 
     
        
     */    
    class Window : public CustomPanel {
    public:

        Widget const * contents() const {
            return contents_;
        }

        Widget * cotents() {
            return contents_;
        }

    protected:

        Window():
            contents_{nullptr},
            modalPane_{new ModalPane{}} {
            setLayout(new MaximizeLayout{});
            add(modalPane_);
            // set the window itself as focusable so that it can accept keyboard events if no-one else does
            setFocusable(true);
        }

        /** Sets the contents of the window. 
         
            Note that the old contents, if any, is *not* deleted by this method. 

            TODO is this correct (least surprise?) behavior? 
         */
        virtual void setContents(Widget * value) {
            if (contents_ != value) {
                if (contents_ != nullptr)
                    remove(contents_);
                contents_ = value;
                // because the modal pane is always over the contents
                addBack(contents_);
            }
        }

        /** Shows the given widget modally. 
         */
        void showModal(Widget * w) {
            modalPane_->add(w);
        }

        /** Shows an error box. 
         */
        void showError(std::string const & error) {
            sendEvent([this, error]() {
                ErrorDialog * e = new ErrorDialog{error};
                modalPane_->add(e);
            });
        }

    private:
        Widget * contents_;
        ModalPane * modalPane_;

    }; // ui::Window

} // namespace ui
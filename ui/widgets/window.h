#pragma once

#include "../widget.h"

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
            contents_{nullptr} {
            setLayout(new Layout::Maximized{});
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
            NOT_IMPLEMENTED;
            //modalPane_->add(w);
        }

        /** Shows an error box. 
         */
        void showError(std::string const & error) {
            NOT_IMPLEMENTED;
            /*
            sendEvent([this, error]() {
                ErrorDialog * e = new ErrorDialog{error};
                modalPane_->add(e);
            });
            */
        }


    private:

        Widget * contents_;

    }; // ui::Window

} // namespace ui
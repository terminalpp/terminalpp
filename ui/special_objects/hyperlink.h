#pragma once

#include "helpers/helpers.h"

#include "../canvas.h"

namespace ui {

    class Hyperlink : public Canvas::SpecialObject {
    public:

        using Ptr = Canvas::SpecialObject::Ptr<Hyperlink>;

        Hyperlink(std::string const & url):
            url_{url} {
        } 

        std::string const & url() const {
            return url_;
        }

        void setUrl(std::string const & url) {
            url_ = url;
        }

        bool active() const {
            return active_;
        }

        void setActive(bool value = true) {
            active_ = value;
        }

    protected:

        void updateFallbackCell(Canvas::Cell & fallback, Canvas::Cell const & original) override {
            MARK_AS_UNUSED(original);
            fallback.font().setUnderline().setDashed();
            if (active())
                fallback.setFg(Color::Blue);
        }

    private:

        std::string url_;

        bool active_ = false;

    }; // ui::Hyperlink

}
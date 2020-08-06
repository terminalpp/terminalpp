#pragma once

#include "ui/mixins/selection_owner.h"

#include "csi_sequence.h"

namespace ui {

    class AnsiTerminal : public virtual Widget, public SelectionOwner {
    public:
        class Palette;


    }; // ui::AnsiTerminal

    // ============================================================================================

    /** Palette
     */
    class AnsiTerminal::Palette {
    public:

        static Palette Colors16();
        static Palette XTerm256(); 

        Palette():
            size_{2},
            defaultFg_{1},
            defaultBg_{0},
            colors_{new Color[2]} {
            colors_[0] = Color::Black;
            colors_[1] = Color::White;
        }

        Palette(size_t size, size_t defaultFg = 15, size_t defaultBg = 0):
            size_(size),
            defaultFg_(defaultFg),
            defaultBg_(defaultBg),
            colors_(new Color[size]) {
            ASSERT(defaultFg < size && defaultBg < size);
        }

        Palette(std::initializer_list<Color> colors, size_t defaultFg = 15, size_t defaultBg = 0);

        Palette(Palette const & from);

        Palette(Palette && from);

        ~Palette() {
            delete [] colors_;
        }

        Palette & operator = (Palette const & other);
        Palette & operator == (Palette && other);

        size_t size() const {
            return size_;
        }

        Color defaultForeground() const {
            return colors_[defaultFg_];
        }

        Color defaultBackground() const {
            return colors_[defaultBg_];
        }

        void setDefaultForegroundIndex(size_t value) {
            defaultFg_ = value;
        }

        void setDefaultBackgroundIndex(size_t value) {
            defaultBg_ = value;
        }

        void setColor(size_t index, Color color) {
            ASSERT(index < size_);
            colors_[index] = color;
        }

        Color operator [] (size_t index) const {
            ASSERT(index < size_);
            return colors_[index];
        } 

        Color & operator [] (size_t index) {
            ASSERT(index < size_);
            return colors_[index];
        } 

        Color at(size_t index) const {
            return (*this)[index];
        }
        Color & at(size_t index) {
            return (*this)[index];
        }

    private:

        size_t size_;
        size_t defaultFg_;
        size_t defaultBg_;
        Color * colors_;

    }; // AnsiTerminal::Palette


} // namespace ui
#pragma once

#include <atomic>

#include "helpers/helpers.h"
#include "geometry.h"

namespace ui {


    /** UI backing buffer, a 2D array of cells. 
     */
    class Buffer final {
    public:

        class CellSpecialObject;
        class Cell;

        explicit Buffer(Size const & size):
            size_{size} {
            create(size_);
        }

        Buffer(Buffer && from) noexcept:
            size_{from.size_},
            rows_{from.rows_} {
            from.size_ = Size{0,0};
            from.rows_ = nullptr;
        }

        Buffer & operator = (Buffer && from) noexcept {
            clear();
            size_ = from.size_;
            rows_ = from.rows_;
            from.size_ = Size{0,0};
            from.rows_ = nullptr;
            return *this;
        }

        ~Buffer() {
            clear();
        }

        Size const & size() const {
            return size_;
        }

        int width() const {
            return size_.width(); 
        }

        int height() const {
            return size_.height();
        }

        bool contains(Point const & x) const {
            return Rect{size_}.contains(x);
        }

        Cell const & operator [] (Point at) const;

        Cell & operator [] (Point at);

    private:

        void create(Size const & size);

        void clear();

        Size size_;

        Cell ** rows_ = nullptr;
    }; // ui::Buffer

    class Buffer::CellSpecialObject {
    public:
        template<typename T = CellSpecialObject>
        class Ptr {
            static_assert(std::is_base_of<CellSpecialObject, T>::value, "Only cell special objects can be used in the smart pointer");

        public:

            explicit Ptr(T * so = nullptr) {
                attach(so);
            }

            Ptr(Ptr const & from) {
                attach(from.ptr_);
            }

            Ptr(Ptr && from): 
                ptr_{from.ptr_} {
                from.ptr_ = nullptr;
            }

            ~Ptr() {
                detach();
            }

            Ptr & operator = (Ptr const & other) {
                if (ptr_ != other.ptr_) {
                    detach();
                    attach(other.ptr_);
                }
            }

            Ptr & operator = (Ptr && other) {
                detach();
                ptr_ = other.ptr_;
                other.ptr_ = nullptr;
                return *this;
            }

            T & operator * () {
                return *ptr_;
            }

            T * operator -> () {
                return ptr_;
            }

            operator T * () const {
                return ptr_;
            }

        private:

            void attach(T * so) {
                if (so != nullptr) {
                    ptr_ = so;
                    ++(so->cells_);
                }
            }

            void detach() {
                if (ptr_ != nullptr) {
                    if (--(ptr_->cells_) == 0)
                        delete ptr_;
                    ptr_ = nullptr;
                }
            }

            T * ptr_ = nullptr;

        }; // ui::Buffer::CellSpecialObject

#ifdef NDEBUG
        virtual ~CellSpecialObject() = default;
#else
        virtual ~CellSpecialObject() noexcept(false) {
            ASSERT(cells_ == 0);
        }
#endif

    protected:

        /** Updates the fallback cell for the special object. 
         
            When a cell is copied and the attached special object is stripped from the copy, this function is called giving the fallback cell to be modified and the original cell as a reference. 

            Special object implementations may decide to implement this feature to change the appearance of the cells. This is also useful for renderers that do not know how to render the particular special object. 
         */
        virtual void updateFallbackCell(Cell & fallback, Cell const & original) {
            MARK_AS_UNUSED(fallback);
            MARK_AS_UNUSED(original);
        }

    private: 

        std::atomic<unsigned> cells_ = 0;

    }; // ui::Buffer::CellSpecialObject

    /** Single cell in a buffer. 
     
        Each cell defines a codepoint, foreground, background and decoration (underline, strikethrough, etc.) color, font and border. In addition to these, a cell may also point to a special object containg extra information. 
     */
    class Buffer::Cell final {
    public:
        using SpecialObject = CellSpecialObject;

        static constexpr char32_t UNICODE_MASK = 0x1fffff;

        Cell(char32_t codepoint = ' '):
            codepoint_{codepoint} {
            ASSERT(codepoint < UNICODE_MASK);
        }

        char32_t codepoint() const {
            return codepoint_ & UNICODE_MASK;
        }

        Cell & setCodepoint(char32_t codepoint) {
            ASSERT(codepoint < UNICODE_MASK);
            codepoint_ = (codepoint & ~UNICODE_MASK) + codepoint;
            return *this;
        }

        Color const & fg() const {
            return fg_;
        }

        Cell & setFg(Color const & value) {
            fg_ = value;
            return *this;
        }

        Color const & bg() const {
            return bg_;
        }

        Cell & setBg(Color const & value) {
            bg_ = value;
            return *this;
        }

        Color const & decor() const {
            return decor_;
        }

        Cell & setDecor(Color const & value) {
            decor_ = value;
            return *this;
        }

        Font const & font() const {
            return font_;
        }

        Cell & setFont(Font const & value) {
            font_ = value;
            return *this;
        }

        Border const & border() const {
            return border_;
        }

        Cell & setBorder(Border const & value) {
            border_ = value;
            return *this;
        }

        SpecialObject * specialObject() const {
            return specialObject_;
        }

        Cell const & setSpecialObject(SpecialObject * so) {
            specialObject_ = SpecialObject::Ptr<>{so};
            return *this;
        }

    private:

        /** The unicode codepoint stored in the cell. 
         
            Since there is only 10ffff characters in unicode this leaves us with 11 bits of extra information that can be stored in a cell. 
         */
        char32_t codepoint_;
        /** Foreground color. 
         */
        Color fg_;
        /** Background color. 
         */
        Color bg_;
        /** Decorations color (underline, strikethrough, etc.)
         */
        Color decor_;
        /** Font used for rendering.
         */
        Font font_;
        /** Border to be displayed around the cell. 
         */
        Border border_;

        /** Pointer to the cell's special object, if any. 
         */
        SpecialObject::Ptr<> specialObject_;

    }; // ui::Buffer::Cell

    inline Buffer::Cell const & Buffer::operator [] (Point at) const {
        ASSERT(contains(at));
        return rows_[at.y()][at.x()];
    }

    inline Buffer::Cell & Buffer::operator [] (Point at) {
        ASSERT(contains(at));
        return rows_[at.y()][at.x()];
    }


} // namespace ui
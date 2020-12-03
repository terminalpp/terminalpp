#pragma once

#include "font.h"
#include "color.h"
#include "border.h"
#include "geometry.h"

namespace ui {

    class Widget;
    class Renderer;

    class Canvas {
        friend class Widget;
        friend class Renderer;
    public:

        class Cursor;
        class SpecialObject;
        class Cell;
        class Buffer;

        explicit Canvas(Buffer & buffer);

        Rect rect() const {
            return Rect{size()};
        }

        Rect visibleRect() const {
            return visibleArea_.rect();
        }

        Size size() const {
            return size_;
        }

        int width() const {
            return size_.width();
        }

        int height() const {
            return size_.height();
        }

        Cursor cursor() const;

        void setCursor(Cursor const & cursor, Point position);

        Point cursorPosition() const;

        /** \name Text metrics
         */
        //@{

        /** Information about a single line of text. 
         */
        struct TextLine {
            /** Width of the line in cells for single-width font of size 1. 
             */
            int width; 
            /** The actual number of codepoints in the line.
             */
            int chars;
            /** First character of the line.
             */
            Char::iterator_utf8 begin;
            /** End of the line (exclusive). 
             */
            Char::iterator_utf8 end;


        }; // Canvas::TextLine

        static constexpr int NoWordWrap = -1;

        static std::vector<TextLine> GetTextMetrics(std::string const & text, int wordWrapAt = NoWordWrap);

        static TextLine GetTextLine(Char::iterator_utf8 & begin, Char::iterator_utf8 const & end, int wordWrapAt = NoWordWrap);

        //@}

        /** \name State
         */
        //@{

        Color const & fg() const {
            return fg_;
        }

        void setFg(Color value) {
            fg_ = value;
        }

        Color const & bg() const {
            return bg_;
        }

        void setBg(Color value) {
            bg_ = value;
        }

        Font const & font() const {
            return font_;
        }

        Font & font() {
            return font_;
        }

        void setFont(Font value) {
            font_ = value;
        }

        //@}

        /** \name Drawing
         */
        //@{

        /** Draws the buffer starting from given top left corner. 
         */
        Canvas & drawBuffer(Buffer const & buffer, Point at);

        /** Draws the fallback buffer, starting from given top left corner. 
         
            Works identically to drawBuffer(), if a cell in the source buffer has special object attached, it is not copied, but the cell is decorated using the special object's fallback.
         */
        Canvas & drawFallbackBuffer(Buffer const & buffer, Point at);

        Canvas & fill(Rect const & rect) {
            return fill(rect, bg_);
        }

        Canvas & fill(Rect const & rect, Color color);

        /** Fills the given rectangle with the specified cell. 
         
            Overrides any previous information, even if the cell would be transparent. 
         */
        Canvas & fill(Rect const & rect, Cell const & fill);

        Canvas & textOut(Point x, std::string const & str) {
            return textOut(x, Char::BeginOf(str), Char::EndOf(str));
        }

        Canvas & textOut(Point x, Char::iterator_utf8 begin, Char::iterator_utf8 end);


        Canvas & setBorder(Point at, Border const & border);
        Canvas & setBorder(Point from, Point to, Border const & border);
        Canvas & setBorder(Rect const & rect, Border const & border);


        Canvas & verticalScrollbar(int size, int offset);
        Canvas & horizontalScrollbar(int size, int offset);

        //@}

        /** \name Single cell access. 
         */
        //@{

        Cell & at(Point const & coords);
        Cell const & at(Point const & coords) const;

        //@}

        /** \name Helpers
         
            Don't really know where to put these...
         */
        //@{

        static std::pair<int, int> ScrollBarDimensions(int length, int max, int offset) {
            int sliderSize = std::max(1, length * length / max);
            int sliderStart = (offset + length == max) ? (length - sliderSize) : (offset * length / max);
            // make sure that slider starts at the top only if we are really at the top
            if (sliderStart == 0 && offset != 0)
                sliderStart = 1;
            // if the slider would go beyond the length, adjust the slider start
            if (sliderStart + sliderSize > length)
                sliderStart = length - sliderSize;
            return std::make_pair(sliderStart, sliderStart + sliderSize);
        }	
        //@}



    private:


        Color fg_;
        Color bg_;
        Color decor_;
        Font font_;

    protected:

        /** Visible area of the canvas. 
         
            Each widget remembers its visible area, which consists of the pointer to its renderer, the offset of the widget's top-left corner in the renderer's absolute coordinates and the area of the widget that translates to a portion of the renderer's buffer. 
        */
        class VisibleArea {
        public:


            VisibleArea() = default;

            VisibleArea(VisibleArea const & ) = default;

            VisibleArea(Point const & offset, Rect const & rect):
                offset_{offset},
                rect_{rect} {
            }

            VisibleArea & operator = (VisibleArea const &) = default;

            /** The offset of the canvas' coordinates from the buffer ones, 
             
                Corresponds to the buffer coordinates of canvas' [0,0].
             */
            Point offset() const {
                return offset_;
            }

            /** The rectangle within the canvas that is backed by the buffer, in the canvas' coordinates. 
             */
            Rect const & rect() const {
                return rect_;
            }

            /** The visible are in buffer coordinates. 
             */
            Rect bufferRect() const {
                return rect_ + offset_;
            }

            VisibleArea clip(Rect const & rect) const {
                return VisibleArea{offset_ + rect.topLeft(), (rect_ & rect) - rect.topLeft()};
            }

            VisibleArea offset(Point const & by) const {
                return VisibleArea{offset_ - by, rect_ + by};
            }

        private:

            Point offset_;
            Rect rect_;
        }; // ui::Canvas::VisibleArea

        /** Creates canvas for given widget. 
         */
        Canvas(Buffer & buffer, VisibleArea const & visibleArea, Size const & size);

    private:

        VisibleArea visibleArea_;
        // we need to support assignment on canvas so can't use reference
        Buffer * buffer_;
        Size size_;

    }; // ui::Canvas

    /** Cursor appearance.
     
        Specifies the appearance of the cursor, such as codepoint and cursor color and whether the cursor is blinking or visible. 
     */
    class Canvas::Cursor {
    public:

        Cursor():
            codepoint_{0x2581},
            visible_{true},
            blink_{true},
            color_{Color::White} {
        }

        char32_t const & codepoint() const {
            return codepoint_;
        }

        bool const & visible() const {
            return visible_;
        }

        bool const & blink() const {
            return blink_;
        }

        Color const & color() const {
            return color_;
        }

        Cursor & setCodepoint(char32_t value) {
            codepoint_ = value;
            return *this;
        }

        Cursor & setVisible(bool value = true) {
            visible_ = value;
            return *this;
        }

        Cursor & setBlink(bool value = true) {
            blink_ = value;
            return *this;
        }

        Cursor & setColor(Color value) {
            color_ = value;
            return *this;
        }

    private:
        char32_t codepoint_;
        bool visible_;
        bool blink_;
        Color color_;
    }; // ui::Canvas::Cursor

    /** Base for objects carrying special cell information. 
     
        Canvas cells can be attached to the special objects, which may contain arbitrary extra information about the cells. The special objects are references counted based on the number of cells that point to them. The Ptr<T> pointer can also be used to hold smart objects safely outside of a cell. 

        When cells are copied from buffer to buffer, the special object's attachment may either be preserved (default) or stripped, in which case the special object is given a chance to alter the copied cell *after* the copy via the updateFallbackCell() method. 

        Special object manipulation (i.e. attaching and detaching from cells and pointers) is thread safe as long as the cell or pointer access is thread safe (the pointer or the cell cannot be accessed concurrently, but two unrelated cells or pointers can attach and detach to the same special object).

        Internally, the SpecialObject holds a map from cell addresses to special objects for the cells that have special object attached. Each cell then has a bit flag that determines whether an attachment to special object exists, or not. 
     */
    class Canvas::SpecialObject {
        friend class Cell;
    public:

        /** Reference counted pointer to a special object of given type. 

            It is assumed that special object implementations will use the specialized version of the pointer as their Ptr typedef, i.e.:

                class SOImpl : public Canvas::SpecialObject {
                public:
                    using Ptr = Canvas::SpecialObject::Ptr<SOImpl>;
                };
         */
        template<typename T> 
        class Ptr {
        public:
            static_assert(std::is_base_of<SpecialObject, T>::value, "Only SpecialObjects can be used in SpecialObject::Ptr");

            explicit Ptr(T * so = nullptr) {
                attach(so);
            }

            Ptr(Ptr const & from) {
                attach(from.ptr_);
            }

            ~Ptr() {
                detach();
            }

            Ptr & operator = (Ptr const & other) {
                if (ptr_ != other.ptr_) {
                    std::lock_guard<std::mutex> g{SpecialObject::MObjects_};
                    if (ptr_ != nullptr && --ptr_->refCount_ == 0)
                        delete ptr_;
                    ptr_ = other.ptr_;
                    if (ptr_ != nullptr)
                        ++ptr_->refCount_;
                }
                return *this;
            }

            Ptr & operator = (T * other) {
                if (ptr_ != other) {
                    std::lock_guard<std::mutex> g{SpecialObject::MObjects_};
                    if (ptr_ != nullptr && --(ptr_->refCount_) == 0)
                        delete ptr_;
                    ptr_ = other;
                    if (ptr_ != nullptr)
                        ++ptr_->refCount_;
                }
                return *this;
            }

            T & operator * () {
                return *ptr_;
            }

            T * operator -> () {
                return ptr_;
            }

            operator T * () {
                return ptr_;
            }

        private:

            void attach(T * so) {
                if (so != nullptr) {
                    std::lock_guard<std::mutex> g{SpecialObject::MObjects_};
                    ++(so->refCount_);
                }
                ptr_ = so;
            }

            void detach() {
                if (ptr_ == nullptr)
                    return;
                std::lock_guard<std::mutex> g{SpecialObject::MObjects_};
                if (--(ptr_->refCount_) == 0)
                    delete ptr_;
            }

            T * ptr_;

        }; // ui::Canvas::SpecialObject::Ptr

        /** Virtual destructor so that special objects do not leak when destroyed. 
         */
        virtual ~SpecialObject() = default;

        /** Detaches the object from all its cells. 
         
            If the object is not referenced from anywhere else, deletes the object at the end. 
         */
        void detachFromAllCells();

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

        /** Number of references (cells and Ptr's) that point to the special object. 
         */
        size_t refCount_ = 0;

        /** Map of reachable special objects. 
         */
        static std::unordered_map<Cell *, SpecialObject *> Objects_;

        /** Guard for multi-threaded access to the special object. 
         */
        static std::mutex MObjects_;

    }; // ui::Canvas::SpecialObject

    /** Canvas cell. 
     
        Each cell contains all drawable information about a single character, i.e. its codepoints, colors, effects, font, borders, etc. Additionally a cell can be attached to a special object, which may provide further information abouty the cell's visual appearance, or behavior. For more details see ui::Canvas::SpecialObject. 
     */
    class Canvas::Cell {
        friend class Canvas::Buffer;
        friend class Canvas::SpecialObject;
    public:

        /** Default constructor.
         */
        Cell():
            codepoint_{' '},
            fg_{Color::White},
            bg_{Color::Black},
            decor_{Color::White},
            font_{},
            border_{} {
        }

        Cell(Cell const & from):
            codepoint_{from.codepoint_},
            fg_{from.fg_},
            bg_{from.bg_},
            decor_{from.decor_},
            font_{from.font_},
            border_{from.border_} {
            SpecialObject * so = from.specialObject();
            if (so != nullptr) {
                std::lock_guard<std::mutex> g{SpecialObject::MObjects_};
                SpecialObject::Objects_.insert(std::pair<Cell *, SpecialObject*>{this, so});
                ASSERT(codepoint_ & SPECIAL_OBJECT);
                ++so->refCount_;
            }
        }

        /** Destroys the cell. 
         
            While nothing has to be done for normal cell, a special cell must detach and possibly delete the special object it contains.
         */
        ~Cell() {
            if (hasSpecialObject())
                detachSpecialObject();
        }

        /** Assignment between cells. 
         
            If the other cell has a special object attached to it, copies the attachment as well, which makes the assignment operator slightly more complex than a simple memory copy. 

            The code is more complex than necessary so that the whole assignment can be done on a single mutex lock. 
         */
        Cell & operator = (Cell const & other) {
            // don't do anything for autoassign
            if (this == & other)
                return *this;
            bool hadSo = hasSpecialObject();
            // casting to void * so that compiler won't give warnings that non POD object is copied, since we deal with the special object later
            memcpy(static_cast<void*>(this), static_cast<void const *>(& other), sizeof(Cell));
            bool hasSo = hasSpecialObject();
            if (hadSo) {
                std::lock_guard<std::mutex> g{SpecialObject::MObjects_};
                auto i = SpecialObject::Objects_.find(this);
                if (--(i->second->refCount_) == 0)
                    delete i->second;
                if (hasSo) {
                    // had & has
                    auto j = SpecialObject::Objects_.find(const_cast<Cell*>(& other));
                    i->second = j->second;
                    ++(i->second->refCount_);
                } else {
                    // had, but does not - detach
                    SpecialObject::Objects_.erase(i);
                }
            } else if (hasSo) {
                // did not have special object, but now we do have it
                std::lock_guard<std::mutex> g{SpecialObject::MObjects_};
                auto j = SpecialObject::Objects_.find(const_cast<Cell*>(& other));
                SpecialObject::Objects_.insert(std::pair<Cell*, SpecialObject*>{this, j->second});
                ++j->second->refCount_;
            }
            return *this;
        };

        /** Assigns the contents of the argument to itself, stripping any attached special objects. 

            The cell is considered a fallback cell and once the attributes of the original cell are copied, modulo the special object binding, the special object can modify the cell contents via the updateFallbackCell method. 

            If the argument does not have a special object attached, behaves like a normal assignment operator. 
         */
        Cell & stripSpecialObjectAndAssign(Cell const & from) {
            if (& from == this)
                return *this;
            bool hadSo = hasSpecialObject();
            // casting to void * so that compiler won't give warnings that non POD object is copied, since we deal with the special object later
            memcpy(static_cast<void*>(this), static_cast<void const *>(& from), sizeof(Cell));
            {
                std::lock_guard<std::mutex> g{SpecialObject::MObjects_};
                if (hadSo) {
                    auto i = SpecialObject::Objects_.find(this);
                    if (--(i->second->refCount_) == 0)
                        delete i->second;
                    SpecialObject::Objects_.erase(i);
                }
                auto j = SpecialObject::Objects_.find(const_cast<Cell*>(& from));
                if (j != SpecialObject::Objects_.end()) {
                    j->second->updateFallbackCell(*this, from);
                    codepoint_ &= ~ SPECIAL_OBJECT;
                }
            }
            return *this;
        }

        /** Detaches special object attached to the cell, if any. 
         
            Since special objects are reference counted, if this is the last cell to point at the object, the special object itself is deleted. 
         */
        Cell & detachSpecialObject() {
            if (hasSpecialObject()) {
                codepoint_ &= ~ SPECIAL_OBJECT;
                std::lock_guard<std::mutex> g{SpecialObject::MObjects_};
                auto i = SpecialObject::Objects_.find(this);
                ASSERT(i != SpecialObject::Objects_.end());
                if (--(i->second->refCount_) == 0)
                    delete i->second;
                SpecialObject::Objects_.erase(i);
            }
            return *this;
        }

        /** Attaches given special object to the cell. 
         
            If the cell already has a special object attached to it, the old object is detached first. 
         */
        Cell & attachSpecialObject(SpecialObject * so) {
            ASSERT(so != nullptr);
            std::lock_guard<std::mutex> g{SpecialObject::MObjects_};
            if (hasSpecialObject()) {
                auto i = SpecialObject::Objects_.find(this);
                ASSERT(i != SpecialObject::Objects_.end());
                if (i->second != so) {
                    if (--(i->second->refCount_) == 0)
                        delete i->second;
                    i->second = so;
                    ++(i->second->refCount_);
                }
            } else {
                SpecialObject::Objects_.insert(std::pair<Cell *, SpecialObject*>{this, so});
                codepoint_ |= SPECIAL_OBJECT;
                ++so->refCount_;
            }
            return *this;
        }

        /** Returns true if the cell has a special object attached to it. 
         */
        bool hasSpecialObject() const {
            return (codepoint_ & SPECIAL_OBJECT) != 0;
        }

        /** Returns the special object attached to the cell, or nullptr if there is none. 
         */
        SpecialObject * specialObject() const {
            if (!hasSpecialObject()) {
                return nullptr;
            } else {
                std::lock_guard<std::mutex> g{SpecialObject::MObjects_};
                auto i = SpecialObject::Objects_.find(const_cast<Cell*>(this));
                ASSERT(i != SpecialObject::Objects_.end());
                return i->second;
            }
        }

        /** \name Codepoint of the cell. 
         */
        //@{
        char32_t codepoint() const {
            return codepoint_ & 0x1fffff;
        }

        Cell & setCodepoint(char32_t value) {
            codepoint_ = (codepoint_ & 0xffe00000) + (value & 0x1fffff);
            return *this;
        }
        //@}

        /** \name Foreground (text) color. 
         */
        //@{
        Color const & fg() const {
            return fg_;
        }

        Cell & setFg(Color value) {
            fg_ = value;
            return *this;
        }
        //@}

        /** \name Background (fill) color. 
         */
        //@{
        Color const & bg() const {
            return bg_;
        }

        Cell & setBg(Color value) {
            bg_ = value;
            return *this;
        }

        //@}

        /** \name Decoration (underline, strikethrough) color. 
         */
        //@{
        Color const & decor() const {
            return decor_;
        }

        Cell & setDecor(Color value) {
            decor_ = value;
            return *this;
        }
        //@}

        /** \name Font. 
         */
        //@{

        Font const & font() const {
            return font_;
        }

        Font & font() {
            return font_;
        }

        Cell & setFont(Font value) {
            font_ = value;
            return *this;
        }
        //@}

        /** \name Border. 
         */
        //@{
        Border const & border() const {
            return border_;
        }

        Border & border() {
            return border_;
        }

        Cell & setBorder(Border const & value) {
            border_ = value;
            return *this;
        }
        //@}


    private:

        /** Marker indicating that special object is attached to the cell. 
         */
        static const char32_t SPECIAL_OBJECT = 0x80000000;

        /** Codepoint and discriminator between normal and special cell type. 
         */
        char32_t codepoint_;

        Color fg_;
        Color bg_;
        Color decor_;
        Font font_;
        Border border_;

    }; // ui::Canvas::Cell

    class Canvas::Buffer {
    public:

        explicit Buffer(Size const & size):
            size_{size} {
            create(size);
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

        virtual ~Buffer() {
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

        /** Determines whether given point lies within the buffer's area. 
         */
        bool contains(Point const & x) const {
            return Rect{size_}.contains(x);
        }

        void resize(Size const & value) {
            if (size_ == value)
                return;
            clear();
            create(value);
        }

        Cell const & at(int x, int y) const {
            return at(Point{x, y});
        }

        Cell const & at(Point p) const {
            return cellAt(p);
        }

        Cell & at(int x, int y) {
            return at(Point{x, y});
        }

        Cell & at(Point p) {
            Cell & result = cellAt(p);
            // clear the unused bits because of non-const access
            SetUnusedBits(result, 0);
            return result;
        }

        /** Returns the cursor properties. 
         */
        Cursor const & cursor() const {
            return cursor_;
        }

        Cursor & cursor() {
            return cursor_;
        }

        Point cursorPosition() const {
            if (contains(cursorPosition_) && (GetUnusedBits(at(cursorPosition_)) & CURSOR_POSITION) == 0)
                return Point{-1,-1};
            else 
                return cursorPosition_;
        }

        /** Sets the cursor and position. 
         */
        void setCursor(Cursor const & value, Point position) {
            cursor_ = value;
            cursorPosition_ = position;
            if (contains(cursorPosition_))
                SetUnusedBits(at(cursorPosition_), CURSOR_POSITION);
        }

        /** Fills portion of given row with the specified cell. 
         
            Exponentially increases the size of copied cells for performance.
         */
        void fillRow(int row, Cell const & fill, int from, int cols) {
            Cell * r = rows_[row];
            for (int e = from + cols; from < e; ++from)
                r[from] = fill;
            /*
            r[from] = fill;
            int i = 1;
            int next = 2;
            while (next < cols) {
                memcpy(r + from + i, r + from, sizeof(Cell) * i);
                i = next;
                next *= 2;
            }
            memcpy(r + from + i, r + from, sizeof(Cell) * (cols - i));
            */
        }

    protected:

        /*
        Cell ** rows() {
            return rows_;
        }
        */

        Cell const & cellAt(Point const & p) const {
            ASSERT(Rect{size_}.contains(p));
            return rows_[p.y()][p.x()];
        }

        Cell & cellAt(Point const & p) {
            ASSERT(Rect{size_}.contains(p));
            return rows_[p.y()][p.x()];
        }

        /** Returns the value of the unused bits in the given cell's codepoint so that the buffer can store extra information for each cell. 
         */
        static char32_t GetUnusedBits(Cell const & cell) {
            return cell.codepoint_ & 0x7fe00000;
        }

        /** Sets the unused bytes value for the given cell to store extra information by the buffer. 
         */
        static void SetUnusedBits(Cell & cell, char32_t value) {
            cell.codepoint_ = (cell.codepoint_ & 0x801fffff) + (value & 0x7fe00000);
        }

        /** Unused bits flag that confirms that the cell has a visible cursor in it. 
         */
        static char32_t constexpr CURSOR_POSITION = 0x200000;

    protected:

        void create(Size const & size) {
            rows_ = new Cell*[size.height()];
            for (int i = 0; i < size.height(); ++i)
                rows_[i] = new Cell[size.width()];
            size_ = size;
        }

        void clear() {
            // rows can be nullptr if they have been backed up by a swap when resizing
            if (rows_ != nullptr) {
                for (int i = 0; i < size_.height(); ++i)
                    delete [] rows_[i];
                delete [] rows_;
            }
            size_ = Size{0,0};
        }

        Size size_;
        Cell ** rows_;

        Cursor cursor_;
        Point cursorPosition_;

    }; // ui::Canvas::Buffer

    inline Canvas::Canvas(Canvas::Buffer & buffer):
        Canvas(buffer, VisibleArea{Point{0,0}, Rect{buffer.size()}}, buffer.size()) {
    }

    inline Canvas::Cursor Canvas::cursor() const {
        return buffer_->cursor();
    }

    inline void Canvas::setCursor(Cursor const & cursor, Point position) {
        buffer_->setCursor(cursor, position + visibleArea_.offset());
    }

    inline Point Canvas::cursorPosition() const {
        return buffer_->cursorPosition();
    }

    inline Canvas::Cell & Canvas::at(Point const & coords) {
        return buffer_->at(coords + visibleArea_.offset());
    }

    inline Canvas::Cell const & Canvas::at(Point const & coords) const {
        return buffer_->at(coords + visibleArea_.offset());
    }

} // namespace ui
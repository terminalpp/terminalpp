#pragma once

#include "helpers/helpers.h"
#include "helpers/locks.h"

#include "font.h"
#include "color.h"
#include "canvas.h"

namespace ui3 {

    class Widget;

    // ============================================================================================


    // ============================================================================================

    /** Base class for all UI renderer implementations. 
     
        
     */ 
    class Renderer {
        friend class Widget;
    public:
    // ============================================================================================

        class Cursor {
        public:

        private:
            char32_t codepoint_;
            bool visible_;
            bool blink_;
            Color color_;
        }; // ui::Renderer::Cursor

        class Buffer;

        class Cell {
            friend class Buffer;
        public:
            class CodepointSetter {
                friend class Cell;
            public:
                CodepointSetter(CodepointSetter &&) = default;
                CodepointSetter(CodepointSetter const &) = delete;

                CodepointSetter & operator = (char32_t value) {
                    codepoint_ = (codepoint_ & 0xffe00000) + (value & 0x1fffff);
                    return *this;
                }

                operator char32_t () {
                    return codepoint_ & 0x1fffff;
                }

            private:
                CodepointSetter(char32_t & codepoint):
                    codepoint_{codepoint} {
                }
                char32_t & codepoint_;
            };  // Cell::CodepointSetter

            /** Default constructor.
             */
            Cell():
                codepoint_{' '},
                fg_{Color::White},
                bg_{Color::Black},
                decor_{Color::White},
                font_{} {
            }

            /** \name Codepoint of the cell. 
             */
            //@{
            char32_t codepoint() const {
                return codepoint_ & 0x1fffff;
            }

            CodepointSetter codepoint() {
                return CodepointSetter{codepoint_};
            }
            //@}

            /** \name Foreground (text) color. 
             */
            //@{
            Color const & fg() const {
                return fg_;
            }

            Color & fg() {
                return fg_;
            }
            //@}

            /** \name Background (fill) color. 
             */
            //@{
            Color const & bg() const {
                return bg_;
            }

            Color & bg() {
                return bg_;
            }
            //@}

            /** \name Decoration (underline, strikethrough) color. 
             */
            //@{
            Color const & decor() const {
                return decor_;
            }

            Color & decor() {
                return decor_;
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
            //@}
        private:

            char32_t codepoint_;
            Color fg_;
            Color bg_;
            Color decor_;
            Font font_;
        }; // ui::Renderer::Cell

        class Buffer {
        public:

            Buffer(Size const & size):
                size_{size} {
                create(size);
            }

            Buffer(Buffer && from):
                size_{from.size_},
                rows_{from.rows_} {
                from.size_ = Size{0,0};
                from.rows_ = nullptr;
            }

            Buffer & operator = (Buffer && from) {
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

        protected:

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
                return cell.codepoint_ & 0xffe00000;
            }

            /** Sets the unused bytes value for the given cell to store extra information by the buffer. 
             */
            static void SetUnusedBits(Cell & cell, char32_t value) {
                cell.codepoint_ = (cell.codepoint_ & 0x1fffff) + (value & 0xffe00000);
            }

            /** Unused bits flag that confirms that the cell has a visible cursor in it. 
             */
            static char32_t constexpr CURSOR_POSITION = 0x200000;

        private:

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

        }; // ui::Renderer::Buffer





    // ============================================================================================

    /** \name Widget Tree
     */
    //@{

    public: 
    
        Widget * root() const {
            return root_;
        }

    private:
        virtual void widgetDetached(Widget * widget) {
            // TODO this should actually do stuff such as checking that mouse and keyboard focus remains valid, etc. 
            NOT_IMPLEMENTED;
        }

        /** Detaches the subtree of given widget from the renderer. 
         */
        void detachTree(Widget * root) {
            detachWidget(root);
            // now that the whole tree has been detached, fix keyboard & mouse issues, etc. 
            NOT_IMPLEMENTED;
        }

        /** Detaches the given widget by invalidating its visible area, including its entire subtree, when detached, calls the widgetDetached() method.
         */
        void detachWidget(Widget * widget);



        Widget * root_;

    //@}

    // ============================================================================================

    /** \name Layouting and Painting
     */
    //@{

    protected: 

        size_t fps() const {
            return fps_; // only UI thread can change fps, no need to lock
        }

        virtual void setFps(size_t value) {
            if (fps_ == value)
                return;
            if (fps_ == 0) {
                // we need to actually start the thread

            } else {
                std::lock_guard<std::mutex> g{renderGuard_};
                fps_ = value;
            }
        }

        virtual void render(Buffer const & buffer, Rect const & rect) = 0;

        /** Returns the visible area of the entire renderer. 
         */
        Canvas::VisibleArea visibleArea() {
            return Canvas::VisibleArea{this, Point{0,0}, Rect{buffer_.size()}};
        }


    private:

        /** Paints the given widget. 

            */
        void paint(Widget * widget) {
            // if FPS is greater than 0, we don't repaint immediately, but only scheduled the widget repaint
            if (fps_ > 0) {
                std::lock_guard<std::mutex> g{renderGuard_};
                if (renderWidget_ == nullptr) {
                    renderWidget_ = widget;
                } else {
                    // find common root
                    NOT_IMPLEMENTED;
                }
            // otherwise repaint immediately
            } else {
                paintAndRender(widget);
            }
        }

        void paintAndRender(Widget * widget);

        Buffer buffer_;
        PriorityLock bufferLock_;
        size_t fps_;
        Widget * renderWidget_;
        std::thread rendererThread_;
        std::mutex renderGuard_;
        

    //@}


    }; // ui::Renderer

} // namespace ui
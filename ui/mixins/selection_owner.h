#pragma once

#include "../widget.h"
#include "../renderer.h"

namespace ui {


    /** Determines selection coordinates on a widget. 

        The selection is inclusive of start, but exclusive of the end cell in both column and row. 

        TODO for now, this is a simple from - to point selection, but in the future, the selection can actually be rectangular, or so or so...
     */
    class Selection {
    public:

        /** Default constructor creates an empty selection. 
         */
        Selection():
            start_{Point{0, 0}},
            end_{Point{0, 0}} {
        }

        /** Creates a selection between two *inclusive* cells. 
         
            Reorders the cells if necessary. 
         */
        static Selection Create(Point start, Point end) {
            if (end.y() < start.y()) {
                std::swap(start, end);
                end.setX(end.x() - 1);
            } else if (end.y() == start.y() && end.x() < start.x()) {
                std::swap(start, end);
                end.setX(end.x() - 1);
            }
            // because the cells themselves are inclusive, but the selection is exclusive on its end, we have to increment the end cell
            end += Point{1,1};
            return Selection(start, end);
        }

        /** Clears the selection. 
         */
        void clear() {
            start_ = Point{0,0};
            end_ = Point{0, 0};
        }

        /** Returns true if the selection is empty. 
         */
        bool empty() const {
            return start_.y() == end_.y();
        }

        /** Returns the first cell of the selection (inclusive). 
         */
        Point start() const {
            return start_;
        }

        /** Returns the last cell of the selection (exclusive).
         */
        Point end() const {
            return end_;
        }

    private:

        Selection(Point const & start, Point const & end):
            start_{start},
            end_{end} {
        }

        Point start_;
        Point end_;

    }; // ui::Selection

    /** Extends widgets with selction ownership & management. 
     
         
     */
    class SelectionOwner : public virtual Widget {
        friend class Renderer;
    public:

        Selection const & selection() const {
            return selection_;
        }

    protected:

        SelectionOwner() {
            autoScrollTimer_.setInterval(50);
            autoScrollTimer_.setHandler([this]() {
                schedule([this](){
                    Point oldSo = scrollOffset();
                    scrollBy(autoScrollIncrement_);
                    if (scrollOffset() == oldSo)
                        stopAutoScroll();
                });
                return true;
            });
        }

        virtual std::string getSelectionContents() = 0;

        /** Clears the selection. 
         
            The method can be called either by the widget itself when it wishes to give up the selection ownership it has, or by the renderer if the selection ownership of the widget has been invalidated from outside. 

            This function must be overriden in subclasses that support selection ownership and when called, must reset the selection cache (if any) and clear the visual indication of the selection ownership. Finally the base implementation must be called which informs the renderer about the selection clear if necessary. 
            */
        virtual void clearSelection() {
            Renderer * r = renderer();
            selection_.clear();
            selectionStart_ = Point{-1, -1};
            if (r != nullptr && r->selectionOwner_ == this) {
                r->clearSelection(this);
            }
            requestRepaint();
        }

        bool updatingSelection() {
            return selectionStart_.x() != -1;
        }

        void setSelection(std::string const & contents) {
            Renderer * r = renderer();
            if (r != nullptr)
                r->setSelection(contents, this);
        }

        /** Marks the selection on the given canvas. 
         */
        void paintSelection(Canvas & canvas, Color background) {
            if (selection_.empty())
                return;
            if (selection_.start().y() + 1 == selection_.end().y()) {
                canvas.fill(Rect{selection_.start(), selection_.end()}, background);

            } else {
                canvas.fill(Rect{selection_.start(), Point{canvas.width(), selection_.start().y() + 1}}, background);
                canvas.fill(Rect{Point{0, selection_.start().y() + 1}, Size{canvas.width(), selection_.end().y() - selection_.start().y() - 2}}, background);
                canvas.fill(Rect{Point{0, selection_.end().y() - 1}, selection_.end()}, background);
            }
        }

    private:

        Selection selection_;
        Point selectionStart_ = Point{-1, -1};

    /** \name Selection Update
     */
    //@{
    
    protected:

        /** Starts the selection update. 
         
            If the widget already has a non-empty selection, clears the selection first and then resets the selection process.
         */
        void startSelectionUpdate(Point start) {
            if (! selection_.empty())
                clearSelection();
            selectionStart_ = start;
        }

        void updateSelection(Point end, Rect contentsRect) {
            // don't do anything if we are not updating the selection
            if (! updatingSelection())
                return;
            end.setX(std::max(contentsRect.left(), end.x()));
            end.setX(std::min(contentsRect.right() - 1, end.x()));
            end.setY(std::max(contentsRect.top(), end.y()));
            end.setY(std::min(contentsRect.bottom() - 1, end.y()));
            // update the selection and call for repaint
            selection_ = Selection::Create(selectionStart_, end);
            requestRepaint();
        }

        /** Finishes the selection update, obtains its contents and registers itself as the selection owner. 
         */
        void endSelectionUpdate() {
            selectionStart_ = Point{-1, -1};
            if (! selection_.empty()) {
                std::string contents{getSelectionContents()};
                setSelection(contents);
            }
        }

        void cancelSelectionUpdate() {
            if (updatingSelection()) {
                selectionStart_ = Point{-1, -1};
                if (!selection_.empty()) {
                    selection_.clear();
                    requestRepaint();
                }
            }
        }

        /** Sets selection directly. 
         
            Clears any preexisting selection (if any), updates the selection, informs the renderer about selection owner & contents change and repaints the widget. 
         */
        void setSelection(Selection const & selection) {
            if (! selection_.empty())
                clearSelection();
            selection_ = selection;
            endSelectionUpdate();
            requestRepaint();
        }

    //@}      

    /** \name Autoscrolling.
     */
    //@{
    protected:

        void startAutoScroll(Point const & step) {
            autoScrollTimer_.stop();
            autoScrollIncrement_ = step;
            autoScrollTimer_.start();
        }

        void stopAutoScroll() {
            autoScrollTimer_.stop();
        }

        bool autoScrollActive() const {
            return autoScrollTimer_.running();
        }

    private:

        Point autoScrollIncrement_ = Point{0,0};

        Timer autoScrollTimer_;

    //@}

    /** \name Default selection behavior. 
     */
    //@{

    protected:

        void paint(Canvas & canvas) override {
            // TODO get the selection color from style
            if (! selection_.empty())
                paintSelection(canvas, Color::Blue.withAlpha(64));            
        }

        void mouseMove(MouseMoveEvent::Payload & e) override {
            if (updatingSelection()) {
                updateSelection(e->coords + scrollOffset(), contentsRect());
                // if the coordinates are outside the widget, do autoscrolling
                if (!Rect{size()}.contains(e->coords)) {
                    startAutoScroll(Point{
                        e->coords.x() < 0 ? -1 : 1,
                        e->coords.y() < 0 ? -1 : 1,
                    });
                } else {
                    stopAutoScroll();
                }
            }
        }

        void mouseDown(MouseButtonEvent::Payload & e) override {
            if (e->modifiers == Key::None) {
                if (e->button == MouseButton::Left) {
                    startSelectionUpdate(e->coords + scrollOffset());
                } else if (e->button == MouseButton::Wheel) {
                    requestSelectionPaste();
                } else if (e->button == MouseButton::Right && ! selection_.empty()) {
                    setClipboard(getSelectionContents());
                    clearSelection();
                }
            }
        }

        void mouseUp(MouseButtonEvent::Payload & e) override {
            if (e->modifiers == Key::None) {
                if (e->button == MouseButton::Left) {
                    if (autoScrollActive())
                        stopAutoScroll();
                    endSelectionUpdate();
                }
            }
        }

    //@}  
    }; // ui::SelectionOwner

} // namespace ui
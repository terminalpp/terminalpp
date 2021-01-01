#pragma once

namespace ui {

    class Widget;

    /** Represents a size hint for a widget. 
     
        Size hint implementations are responsible for determining the widget's dimensions. A size hint class must provide two methods, the calculateWidth() and calculateHeight() which are called by the layout engines to determine proper sizes of the child widgets. 

        The size hints can be categorized into three broad categories, `Manual`, which ignore the layout and contents of the widget and only use the current widget dimension, `Layout`, whose value is calculated from the layout properties (i.e. available client size, parent size, etc.).

     */
    class SizeHint {
    public:
        /** Basic size hints. 
         */
        class Manual;
        class Percentage;
        class AutoLayout;
        class AutoSize;

        /** Size hint reference kind. 
         
            Specifies what properties is the returned size based on. For Kind::Manual, the widget's actual size is used. For Kind::Auto, the widget's contents is used and for Kind::Layout the parent properties (size, contents size, etc.) and the layout itself are used. 
         */
        enum class Kind {
            Manual,
            Layout,
            Auto,
        };

        virtual ~SizeHint() = default;

        /** Calculates the width of the given widget. 
         
            Takes the default width for the widget calculated by the layout and the total available width within the layout. 
         */
        virtual int calculateWidth(Widget const * widget, int layoutSize, int availableLayoutSize) const = 0;
        virtual int calculateHeight(Widget const * widget, int layoutSize, int availableLayoutSize) const = 0;

        Kind kind() const {
            return kind_;
        }

        bool isManual() const {
            return kind_ == Kind::Manual;
        }

        bool isLayout() const {
            return kind_ == Kind::Layout;
        }

        bool isAuto() const {
            return kind_ == Kind::Auto;
        }

    protected:

        SizeHint(Kind kind): 
            kind_{kind} {
        }

    private:

        Kind const kind_;

    }; 

    class SizeHint::Manual : public SizeHint {
    public:
        Manual():
            SizeHint{Kind::Manual} {
        }

        int calculateWidth(Widget const * widget, int autoSize, int availableSize) const override;
        int calculateHeight(Widget const * widget, int autoSize, int availableSize) const override;
    };

    class SizeHint::Percentage : public SizeHint {
    public:
        Percentage(int value):
            SizeHint{Kind::Layout},
            value_{value} {
        }

        int calculateWidth(Widget const * widget, int autoSize, int availableSize) const override {
            MARK_AS_UNUSED(widget);
            MARK_AS_UNUSED(autoSize);
            return availableSize * value_ / 100;
        }

        int calculateHeight(Widget const * widget, int autoSize, int availableSize) const override {
            MARK_AS_UNUSED(widget);
            MARK_AS_UNUSED(autoSize);
            return availableSize * value_ / 100;
        }

        int value_;
    };

    class SizeHint::AutoLayout : public SizeHint {
    public:
        AutoLayout():
            SizeHint{Kind::Layout} {
        }

        int calculateWidth(Widget const * widget, int autoSize, int availableSize) const override {
            MARK_AS_UNUSED(widget);
            MARK_AS_UNUSED(availableSize);
            return autoSize;
        }

        int calculateHeight(Widget const * widget, int autoSize, int availableSize) const override {
            MARK_AS_UNUSED(widget);
            MARK_AS_UNUSED(availableSize);
            return autoSize;
        }
    };

    class SizeHint::AutoSize : public SizeHint {
    public:
        AutoSize():
            SizeHint{Kind::Auto} {
        }
        int calculateWidth(Widget const * widget, int autoSize, int availableSize) const override;
        int calculateHeight(Widget const * widget, int autoSize, int availableSize) const override;
    };

    /** Layout implementation. 
     */
    class Layout {
    public:

        class None;
        class Maximized;
        class Row;
        class Column;

        virtual ~Layout() = default;

        /** Does the layout, can call move & resize only
         */
        virtual void layout(Widget * widget) const = 0;

        /** Calculates the overlay of immediate children of the given widget. 
         */
        virtual void calculateOverlay(Widget * widget) const;

    protected:

        Size contentsSize(Widget * widget) const;

        std::deque<Widget *> const & children(Widget * widget) const;

        void resize(Widget * widget, Size const & size) const;

        void resize(Widget * widget, int width, int height) const {
            resize(widget, Size{width, height});
        }

        void move(Widget * widget, Point const & topLeft) const;

        void setOverlaid(Widget * widget, bool value) const;

    };

    class Layout::None : public Layout {
    public:

        void layout(Widget * widget) const override {
            MARK_AS_UNUSED(widget);
            // actually do nothing
        }


    }; // Layout::None

    class Layout::Maximized : public Layout {
    public:
        void layout(Widget * widget) const override;

        void calculateOverlay(Widget * widget) const override;

    }; // Layout::Maximized

    class Layout::Row : public Layout {
    public:

        Row(HorizontalAlign hAlign, VerticalAlign vAlign = VerticalAlign::Top):
            hAlign_{hAlign},
            vAlign_{vAlign} {
        }

        void layout(Widget * widget) const override;

        void calculateOverlay(Widget * widget) const override;

    private:
        HorizontalAlign hAlign_;
        VerticalAlign vAlign_;
    }; // Layout::Row

    /** Arranges the widgets in a single column. 
     
     */
    class Layout::Column : public Layout {
    public:
        explicit Column(HorizontalAlign hAlign, VerticalAlign vAlign = VerticalAlign::Top):
            hAlign_{hAlign},
            vAlign_{vAlign} {
        }

        explicit Column(VerticalAlign vAlign = VerticalAlign::Top):
            hAlign_{HorizontalAlign::Center},
            vAlign_{vAlign} {
        }

        void layout(Widget * widget) const override;

        void calculateOverlay(Widget * widget) const override;

    private:
        HorizontalAlign hAlign_;
        VerticalAlign vAlign_;
    }; // Layout::Column



} // namespace ui